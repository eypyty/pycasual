//!
//! manager_handle.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager/state.h"

#include "common/message.h"
#include "common/logger.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/queue.h"

using casual::common::exception::signal::child::Terminate;
using casual::common::process::lifetime;
using casual::common::queue::blocking::basic_reader;




namespace casual
{
   namespace transaction
   {

      namespace policy
      {
         struct Manager : public state::Base
         {
            using state::Base::Base;

            void apply()
            {
               try
               {
                  throw;
               }
               catch( const common::exception::signal::child::Terminate& exception)
               {
                  auto terminated = common::process::lifetime::ended();
                  for( auto& death : terminated)
                  {
                     switch( death.why)
                     {
                        case common::process::lifetime::Exit::Why::core:
                           common::logger::error << "process crashed: TODO: maybe restart? " << death.string();
                           break;
                        default:
                           common::logger::information << "proccess died: " << death.string();
                           break;
                     }

                     state::remove::instance( death.pid, m_state);
                  }
               }
            }
         };
      } // policy


      using QueueBlockingReader = common::queue::blocking::basic_reader< policy::Manager>;
      using QueueNonBlockingReader = common::queue::non_blocking::basic_reader< policy::Manager>;

      using QueueBlockingWriter = common::queue::ipc_wrapper< common::queue::blocking::basic_writer< policy::Manager>>;
      //using QueueNonBlockingWriter = common::queue::ipc_wrapper< common::queue::non_blocking::basic_writer< policy::Manager>>;

      namespace handle
      {



         template< typename BQ>
         struct ResourceConnect : public state::Base
         {
            typedef common::message::transaction::resource::connect::Reply message_type;

            using broker_queue = BQ;

            ResourceConnect( State& state, broker_queue& brokerQueue) : state::Base( state), m_brokerQueue( brokerQueue) {}

            void dispatch( message_type& message)
            {
               common::logger::information << "resource proxy pid: " <<  message.id.pid << " connected";


               auto instance = m_state.instances.find( message.id.pid);

               if( instance != std::end( m_state.instances))
               {
                  if( message.state == XA_OK)
                  {
                     instance->second->state = state::resource::Proxy::Instance::State::idle;
                     instance->second->id = std::move( message.id);

                  }
                  else
                  {
                     common::logger::error << "resource proxy pid: " <<  message.id.pid << " startup error";
                     instance->second->state = state::resource::Proxy::Instance::State::startupError;
                     //throw common::exception::signal::Terminate{};
                     // TODO: what to do?
                  }
               }
               else
               {
                  common::logger::error << "transaction manager - unexpected resource connecting - pid: " << message.id.pid << " - action: discard";
               }

               if( ! m_connected && std::all_of( std::begin( m_state.resources), std::end( m_state.resources), state::filter::Running{}))
               {
                  //
                  // We now have enough resource proxies up and running to guarantee consistency
                  // notify broker
                  //
                  common::message::transaction::Connected running;
                  m_brokerQueue( running);

                  m_connected = true;
               }
            }
         private:
            broker_queue& m_brokerQueue;
            bool m_connected = false;

         };

         template< typename BQ>
         ResourceConnect< BQ> resourceConnect( State& state, BQ&& brokerQueue)
         {
            return ResourceConnect< BQ>{ state, std::forward< BQ>( brokerQueue)};
         }


         //using ResourceConnect = basic_resource_connect< QueueBlockingWriter>;

         struct Begin : public state::Base
         {
            typedef common::message::transaction::begin::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

               m_state.log.begin( message);

               state::pending::Reply reply;
               reply.target = message.id.queue_id;

               m_state.pendingReplies.push_back( std::move( reply));
            }
         };

         struct Commit : public state::Base
         {
            typedef common::message::transaction::commit::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

            }
         };

         struct Rollback : public state::Base
         {
            typedef common::message::transaction::rollback::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

            }
         };

         struct Involved : public state::Base
         {
            typedef common::message::transaction::resource::Involved message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               auto task = std::find_if( std::begin( m_state.tasks), std::end( m_state.tasks), action::Task::Find( message.xid));

               if( task != std::end( m_state.tasks))
               {
                  std::copy(
                     std::begin( message.resources),
                     std::end( message.resources),
                     std::back_inserter( task->resources));

                  std::stable_sort( std::begin( task->resources), std::end( task->resources));

                  task->resources.erase(
                        std::unique( std::begin( task->resources), std::end( task->resources)),
                        std::end( task->resources));

               }
               // TODO: else, what to do?
            }
         };


      } // handle
   } // transaction


} // casual

#endif // MANAGER_HANDLE_H_
