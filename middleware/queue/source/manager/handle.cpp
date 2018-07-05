//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/handle.h"
#include "queue/common/log.h"
#include "queue/manager/admin/server.h"

#include "common/exception/system.h"
#include "common/exception/casual.h"
#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/message/handle.h"
#include "common/event/listen.h"




namespace casual
{
   namespace queue
   {
      namespace manager
      {

         namespace ipc
         {
            const common::communication::ipc::Helper& device()
            {
               static common::communication::ipc::Helper ipc{
                  common::communication::error::handler::callback::on::Terminate
                  {
                     []( const common::process::lifetime::Exit& exit){
                        //
                        // We put a dead process event on our own ipc device, that
                        // will be handled later on.
                        //
                        common::message::event::process::Exit event{ exit};
                        common::communication::ipc::inbound::device().push( std::move( event));
                     }
                  }
               };
               return ipc;
            }

         } // ipc

         namespace handle
         {

            namespace local
            {
               namespace
               {
                  template< typename G, typename M>
                  void send( State& state, G&& groups, M&& message)
                  {
                     // TODO: until we get "auto lambdas"
                     using group_type = decltype( *std::begin( groups));

                     //
                     // Try to send it first with no blocking.
                     //

                     auto busy = common::algorithm::partition( groups, [&]( group_type& g)
                           {
                              return ! ipc::device().non_blocking_send( g.queue, message);
                           });

                     //
                     // Block for the busy ones, if any
                     //
                     for( auto&& group : std::get< 0>( busy))
                     {
                        ipc::device().blocking_send( group.queue, message);
                     }

                  }

                  namespace optional
                  {
                     template< typename D, typename M>
                     bool send( D&& device, M&& message)
                     {
                        try
                        {
                           ipc::device().blocking_send( device, std::forward< M>( message));
                           return true;
                        }
                        catch( const common::exception::system::communication::Unavailable&)
                        {
                           return false;
                        }
                     }
                  } // optional

                  template< typename D, typename M>
                  void reply( D&& device, M&& message)
                  {
                     if( ! optional::send( std::forward< D>( device), std::forward< M>( message)))
                     {
                        common::log::category::error << "device [" << device << "] unavailable for reply - action: ignore\n";
                     }
                  }



               } // <unnamed>
            } // local

            namespace process
            {
               void Exit::operator () ( message_type& message)
               {
                  apply( message.state);
               }

               void Exit::apply( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "handle::process::Exit"};

                  m_state.remove( exit.pid);

               }
            } // process

            namespace shutdown
            {
               void Request::operator () ( message_type& message)
               {
                  std::vector< common::process::Handle> groups;
                  common::algorithm::transform( m_state.groups, groups, std::mem_fn( &manager::State::Group::process));

                  common::algorithm::for_each(
                        common::server::lifetime::soft::shutdown( groups, std::chrono::seconds( 1)),
                        std::bind( &process::Exit::apply, process::Exit{ m_state}, std::placeholders::_1));

                  throw common::exception::casual::Shutdown{};
               }

            } // shutdown

            namespace lookup
            {

               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "handle::lookup::Request"};

                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);


                  auto send_reply = common::execute::scope( [&](){
                     local::reply( message.process.ipc, reply);
                  });

                  auto found =  common::algorithm::find( m_state.queues, message.name);

                  if( found && ! found->second.empty())
                  {
                     log << "queue found: " << common::range::make( found->second) << '\n';

                     auto& queue = found->second.front();
                     reply.queue = queue.queue;
                     reply.process = queue.process;
                     reply.order = queue.order;
                  }
                  else
                  {
                     common::log::line( log, "queue not found - ", message.name);
                     //
                     // TODO: Check if we have already have pending request for this queue.
                     // If so, we don't need to ask again.
                     // not sure if the semantics holds, so I don't implement it until we know.
                     //

                     //
                     // We didn't find the queue, let's ask our neighbors.
                     //

                     common::message::gateway::domain::discover::Request request;
                     request.correlation = message.correlation;
                     request.domain = common::domain::identity();
                     request.process = common::process::handle();
                     request.queues.push_back(  message.name);

                     if( local::optional::send( common::communication::ipc::gateway::manager::optional::device(), std::move( request)))
                     {
                        m_state.pending.push_back( std::move( message));

                        log << "pending request added to pending: " << common::range::make( m_state.pending) << '\n';

                        //
                        // We don't send reply, we'll do it when we get the reply from the gateway.
                        //
                        send_reply.release();
                     }
                  }
               }

            } // lookup

            namespace connect
            {
               void Request::operator() ( common::message::queue::connect::Request& request)
               {
                  auto found = common::algorithm::find( m_state.groups, request.process.pid);

                  if( found)
                  {
                     auto& group = *found;

                     auto reply = common::message::reverse::type( request);
                     reply.name = group.name;

                     auto configuration = m_state.group_configuration( group.name);

                     if( configuration)
                     {
                        common::algorithm::transform( configuration->queues, reply.queues, []( auto& value){
                           common::message::queue::Queue result;
                           result.name = value.name;
                           result.retries = value.retries;
                           return result;
                        });
                     }
                     else
                     {
                        common::log::line( common::log::category::error, "failed to correlate configuration for group - ", group.name);
                     }

                     ipc::device().blocking_send( request.process.ipc, reply);
                  }
                  else
                  {
                     common::log::line( common::log::category::error, "failed to correlate queue group - ", request.process.pid);
                  }
               }


               void Information::operator () ( common::message::queue::Information& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     auto& instances = m_state.queues[ queue.name];

                     instances.emplace_back( message.process, queue.id);

                     common::algorithm::stable_sort( instances);
                  }

                  auto found = common::algorithm::find( m_state.groups, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->connected = true;
                  }
                  else
                  {
                     //
                     // We add the group
                     //
                     m_state.groups.emplace_back( message.name, message.process);
                  }

               }



            } // connect


            namespace domain
            {
               void Advertise::operator () ( message_type& message)
               {
                  Trace trace{ "handle::domain::Advertise"};

                  log << "message: " << message << '\n';

                  m_state.update( message);

                  using directive_type = decltype( message.directive);

                  if( common::compare::any( message.directive, { directive_type::add, directive_type::replace}))
                  {
                     //
                     // Queues has been added, we check if there are any pending
                     //

                     auto split = common::algorithm::stable_partition( m_state.pending,[&]( auto& p){

                        return ! common::algorithm::any_of( message.queues, [&]( auto& q){
                           return q.name == p.name;});
                     });

                     common::traits::concrete::type_t< decltype( m_state.pending)> pending;

                     common::algorithm::move( std::get< 1>( split), pending);
                     common::algorithm::trim( m_state.pending, std::get< 0>( split));

                     log << "pending to lookup: " << common::range::make( pending) << '\n';

                     common::algorithm::for_each( pending, [&]( auto& pending){
                        lookup::Request{ m_state}( pending);
                     });
                  }
               }


               namespace discover
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "handle::domain::discover::Request"};

                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     reply.process = common::process::handle();
                     reply.domain = common::domain::identity();

                     for( auto& queue : message.queues)
                     {
                        if( common::algorithm::find( m_state.queues, queue))
                        {
                           reply.queues.emplace_back( queue);
                        }
                     }

                     common::log::line( verbose::log, "reply: ", reply);

                     ipc::device().blocking_send( message.process.ipc, reply);
                  }

                  void Reply::operator () ( message_type& message)
                  {
                     Trace trace{ "handle::domain::discover::Reply"};

                     //
                     // outbound has already advertised the queues (if any), so we have that handled
                     // check if there are any pending lookups for this reply
                     //

                     auto found = common::algorithm::find_if( m_state.pending, [&]( const auto& r){
                        return r.correlation == message.correlation;
                     });

                     if( found)
                     {
                        auto request = std::move( *found);
                        m_state.pending.erase( std::begin( found));

                        auto reply = common::message::reverse::type( *found);

                        auto found_queue = common::algorithm::find( m_state.queues, request.name);

                        if( found_queue && ! found_queue->second.empty())
                        {
                           auto& queue = found_queue->second.front();
                           reply.process = queue.process;
                           reply.queue = queue.queue;
                        }

                        local::reply( request.process.ipc, reply);
                     }
                     else
                     {
                        log << "no pending was found for discovery reply: " << message << '\n';
                     }
                  }
               } // discover
            } // domain
         } // handle

         handle::dispatch_type handlers( State& state)
         {
            return {
               common::event::listener( manager::handle::process::Exit{ state}),
               manager::handle::connect::Information{ state},
               manager::handle::connect::Request{ state},
               manager::handle::shutdown::Request{ state},
               manager::handle::lookup::Request{ state},
               //manager::handle::peek::queue::Request{ m_state},
               manager::handle::domain::Advertise{ state},
               manager::handle::domain::discover::Request{ state},
               manager::handle::domain::discover::Reply{ state},

               common::server::handle::admin::Call{
                  manager::admin::services( state),
                  ipc::device().error_handler()},
               common::message::handle::ping(),
            };
         }

      } // manager
   } // queue
} // casual

