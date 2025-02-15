//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/state.h"
#include "queue/group/handle.h"
#include "queue/common/ipc/message.h"
#include "queue/common/ipc.h"
#include "queue/common/log.h"

#include "common/argument.h"
#include "common/process.h"
#include "common/exception/handle.h"
#include "common/communication/instance.h"
#include "common/message/signal.h"

namespace casual
{
   using namespace common;
   namespace queue::group
   {
         namespace local
         {
            namespace
            { 
               struct Settings
               {
                  CASUAL_LOG_SERIALIZE()
               };

               State initialize( Settings settings)
               {
                  Trace trace{ "queue::group::local::initialize"};

                  // make sure we handle "alarms"
                  signal::callback::registration< code::signal::alarm>( []()
                  {
                     // Timeout has occurred, we push the corresponding 
                     // signal to our own "queue", and handle it "later"
                     ipc::device().push( common::message::signal::Timeout{});
                  });

                  // connect to queue-manager - it will send configuration::update::Reqest that we'll handle
                  // in the main message pump
                  communication::device::blocking::send( 
                     communication::instance::outbound::queue::manager::device(),
                     ipc::message::group::Connect{ process::handle()});

                  return {};
               }

               auto condition( State& state)
               {
                  return common::message::dispatch::condition::compose(
                     common::message::dispatch::condition::done( [&state](){ return state.done();}),
                     common::message::dispatch::condition::idle( [&state]()
                     {
                        // make sure we persist when inbound is idle,
                        handle::persist( state);
                     })
                  );
               }

               void start( State state)
               {
                  Trace trace{ "queue::group::local::start"};

                  auto abort_guard = execute::scope( [&state]()
                  {
                     handle::abort( state);
                  });

                  common::message::dispatch::pump( 
                     condition( state), 
                     group::handlers( state), 
                     ipc::device());

                  abort_guard.release();
               }

               void main( int argc, char **argv)
               {
                  Trace trace{ "queue::group::local::main"};

                  Settings settings;
                  argument::Parse{ "queue group server",
                  }( argc, argv);

                  log::line( verbose::log, "settings: ", settings);

                  start( initialize( std::move( settings)));
               }

            } // <unnamed>
         } // local

   } // queue::group
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::queue::group::local::main( argc, argv);
   });
}
