//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/state.h"
#include "gateway/group/inbound/handle.h"
#include "gateway/group/handle.h"
#include "gateway/group/tcp.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/message/signal.h"
#include "common/message/internal.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace gateway::group::inbound::reverse
   {
      using namespace common;
      
      namespace local
      {
         namespace
         {
            struct Arguments
            {
               // might have som arguments in the future
            };


            // local state to keep additional stuff for reverse connections...
            struct State : inbound::State
            {
               struct Connection
               {
                  Connection( configuration::model::gateway::inbound::Connection configuration)
                     : configuration{ std::move( configuration)} {}

                  configuration::model::gateway::inbound::Connection configuration;

                  struct
                  {
                     platform::size::type attempts{};
                     
                     CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( attempts);)
                  } metric;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( configuration);
                     CASUAL_SERIALIZE( metric);
                  )
               };

               std::vector< Connection> unconnected;

               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( unconnected);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::inbound::reverse::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               ipc::flush::send( ipc::manager::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "gateway::group::inbound::reverse::local::external::connect"};

                  group::tcp::connect< group::tcp::connector::Bound::in>( state, state.unconnected);
                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::inbound::Connection configuration)
               {
                  Trace trace{ "gateway::inbound::local::external::reconnect"};

                  if( state.runlevel == decltype( state.runlevel())::running)
                  {
                     log::line( log::category::information, code::casual::communication_unavailable, " lost connection ", configuration.address, " - action: try to reconnect");
                     state.unconnected.emplace_back( std::move( configuration));
                     external::connect( state);
                  }
               }

               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     return [&state, handler = inbound::handle::external( state)]
                        ( common::strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
                     {
                        if( auto connection = state.external.connection( descriptor))
                        {
                           try
                           {
                              if( auto correlation = handler( connection->next()))
                                 state.correlations.emplace_back( std::move( correlation), descriptor);
                           }
                           catch( ...)
                           {
                              if( exception::capture().code() != code::casual::communication_unavailable)
                                 throw;
                              
                              // Do we try to reconnect?
                              if( auto configuration = handle::connection::lost( state, descriptor))
                                 external::reconnect( state, std::move( configuration.value()));
                           }
                           return true;
                        }
                        return false;
                     };
                  }
               } // dispatch

            } // external

            namespace internal
            {
               // handles that are specific to the reverse-inbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::inbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...
                           state.alias = message.model.alias;
                           state.pending.requests.limit( message.model.limit);
                           
                           state.unconnected = algorithm::transform( message.model.connections, []( auto& configuration)
                           {
                              return local::State::Connection{ std::move( configuration)};
                           });

                           // we might got some addresses to try...
                           external::connect( state);

                           // send reply
                           ipc::flush::optional::send(
                              message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::inbound::reverse::state::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);
                           log::line( verbose::log, "state: ", state);
                           
                           auto reply = state.reply( message);

                           // add reverse connections
                           algorithm::transform( state.unconnected, reply.state.connections, []( auto& pending)
                           {
                              message::inbound::state::Connection result;
                              result.configuration = pending.configuration;
                              result.address.peer = pending.configuration.address;
                              return result;
                           });
                           
                           ipc::flush::optional::send( message.process.ipc, reply);
                        };
                     }

                  } // state

                  namespace shutdown
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::shutdown::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           state.runlevel = decltype( state.runlevel())::shutdown;
                           inbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

                  auto timeout( State& state)
                  {
                     return [&state]( const common::message::signal::Timeout& message)
                     {
                        Trace trace{ "gateway::group::inbound::reverse::local::internal::handle::timeout"};

                        external::connect( state);
                     };
                  }

               } // handle

               auto handler( State& state)
               {
                  // we add the common/general inbound handlers
                  return inbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     common::message::internal::dump::state::handle( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state),
                     handle::timeout( state)
                  );
               }

            } // internal

            namespace signal::callback
            {
               auto timeout()
               {
                  return []()
                  {
                     Trace trace{ "gateway::group::inbound::reverse::local::signal::callback::timeout"};

                     // we push it to our own inbound ipc 'queue', and handle the timeout
                     // in our regular message pump.
                     ipc::inbound().push( common::message::signal::Timeout{});    
                  };
               }
            } // signal::callback

            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state](){ inbound::handle::idle( state);})
                  );
            }

            void run( State state)
            {
               Trace trace{ "gateway::group::inbound::reverse::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::inbound::handle::abort( state);
               });

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout());

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  group::tcp::pending::send::dispatch( state),
                  ipc::dispatch::create( state, &internal::handler),
                  external::dispatch::create( state)
               );

               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Arguments arguments;

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::inbound::reverse

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::inbound::reverse::local::main( argc, argv);
   });
} // main