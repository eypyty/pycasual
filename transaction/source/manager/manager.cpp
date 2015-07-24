//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/manager/admin/server.h"


#include "common/server/handle.h"
#include "common/server/lifetime.h"
#include "common/trace.h"
#include "common/queue.h"
#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/log.h"


#include "config/domain.h"


#include <tx.h>

using namespace casual::common;


extern "C"
{
   extern void casual_listTransactions( TPSVCINFO *serviceInfo);
}


namespace casual
{
   namespace transaction
   {

      Manager::Manager( const Settings& settings) :
          m_queueFilePath( common::process::singleton( common::environment::domain::singleton::path() + "/.casual-transaction-manager-queue")),
          m_state( settings.database)
      {

      }

      Manager::~Manager()
      {
         common::Trace trace{ "transaction::Manager::~Manager", common::log::internal::transaction};

         try
         {
            std::vector< common::process::Handle> instances;

            for( auto& resource : m_state.resources)
            {
               common::range::transform( resource.instances, instances, std::mem_fn( &state::resource::Proxy::Instance::process));
            }

            common::server::lifetime::shutdown( m_state, instances, {}, std::chrono::seconds( 1));

            auto pids  = m_state.processes();

            if( pids.size() != 0)
            {
               common::log::error << "failed to shutdown resource proxies: " << range::make( pids) << " - action: abort" << std::endl;
            }
            else
            {
               common::log::information << "casual-transaction-manager off-line\n";
            }

         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Manager::start()
      {
         try
         {
            auto start = common::platform::clock_type::now();

            common::log::internal::transaction << "transaction manager start\n";



            //
            // Connect and get configuration from broker
            //
            {
               trace::internal::Scope trace( "configure", common::log::internal::transaction);
               action::configure( m_state);
            }

            //
            // Start resource-proxies
            //
            {
               trace::internal::Scope trace( "start rm-proxy-servers", common::log::internal::transaction);

               common::range::for_each(
                  common::range::make( m_state.resources),
                  action::boot::Proxie( m_state));

            }


            auto instances = common::range::accumulate(
                  m_state.resources, 0,
                  []( std::size_t count, const state::resource::Proxy& p)
                  {
                     return count + p.instances.size();
                  }
                  );

            auto end = common::platform::clock_type::now();


            common::log::information << "transaction manager is on-line - "
                  << m_state.resources.size() << " resources - "
                  << instances << " instances - boot time: "
                  << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;


            //
            // We're ready to start....
            //
            message::pump( m_state);


         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
         }
         catch( ...)
         {
            common::error::handler();
         }
      }


      const State& Manager::state() const
      {
         return m_state;
      }

      namespace message
      {
         void pump( State& state)
         {
            auto& ipc = common::ipc::receive::queue();

            try
            {

               common::log::internal::transaction << "prepare message dispatch handlers\n";

               //
               // prepare message dispatch handlers...
               //

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  handle::dead::Process{ state},
                  handle::Begin{ state},
                  handle::Commit{ state},
                  handle::Rollback{ state},
                  handle::resource::Involved{ state},
                  handle::resource::reply::Connect{ state},
                  handle::resource::reply::Prepare{ state},
                  handle::resource::reply::Commit{ state},
                  handle::resource::reply::Rollback{ state},
                  handle::domain::Prepare{ state},
                  handle::domain::Commit{ state},
                  handle::domain::Rollback{ state},
                  handle::domain::resource::reply::Prepare{ state},
                  handle::domain::resource::reply::Commit{ state},
                  handle::domain::resource::reply::Rollback{ state},
                  common::server::handle::basic_admin_call< State>{
                     ipc, admin::services( state), state, common::process::instance::identity::transaction::manager()},
                  common::message::handle::ping( state),

                  //
                  // We discard the connect reply message
                  //
                  common::message::handle::Discard< common::message::server::connect::Reply>{},
               };


               common::log::internal::transaction << "start message pump\n";


               queue::blocking::Reader queueReader{ ipc, state};

               while( true)
               {
                  common::Trace trace{ "transaction::Manager message pump", common::log::internal::transaction};

                  {
                     scoped::Writer batchWrite( state.log);

                     if( ! state.pending())
                     {
                        //
                        // We can only block if our backlog is empty
                        //

                        try
                        {
                           common::signal::timer::Deadline deadline{ state.log.deadline()};

                           handler( queueReader.next());
                        }
                        catch( const exception::signal::Timeout&)
                        {
                           //
                           // We've got a transaction timeout.
                           // casual doesn't really care, but to play nice with RM's and let them release resources
                           // and such, we roll-back every transaction that has reached it's deadline.
                           //
                           action::timeout( state);
                        }
                     }


                     //
                     // Consume until the queue is empty or we've got pending replies equal to batch::transaction
                     //

                     queue::non_blocking::Reader nonBlocking( ipc, state);

                     while( handler( nonBlocking.next()) &&
                           state.persistentReplies.size() < common::platform::batch::transaction)
                     {
                        ;
                     }
                  }

                  //
                  // Send persistent replies to clients
                  //
                  {

                     common::log::internal::transaction << "manager persistent replies: " << state.persistentReplies.size() << "\n";

                     auto notDone = common::range::partition(
                           state.persistentReplies,
                           common::negate( action::persistent::Send{ state}));

                     common::range::trim( state.persistentReplies, std::get< 0>( notDone));

                     common::log::internal::transaction << "manager persistent replies: " << state.persistentReplies.size() << "\n";
                  }

                  //
                  // Send persistent resource requests
                  //
                  {
                     common::log::internal::transaction << "manager persistent request: " << state.persistentRequests.size() << "\n";

                     auto notDone = common::range::partition(
                           state.persistentRequests,
                           common::negate( action::persistent::Send{ state}));

                     //
                     // Move the ones that did not find an idle resource to pending requests
                     //
                     common::range::move( std::get< 0>( notDone), state.pendingRequests);

                     state.persistentRequests.clear();

                  }

                  common::log::internal::transaction << "manager transactions: " << state.transactions.size() << "\n";
               }
            }
            catch( const exception::Shutdown&)
            {
               //
               // We do nothing
               //
            }
         }

      } // message

   } // transaction
} // casual


