//!
//! casual 
//!

#include "domain/delay/delay.h"
#include "domain/delay/message.h"
#include "domain/common.h"

#include "common/server/handle/call.h"
#include "common/arguments.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/communication/ipc.h"


namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace delay
      {

         void start( State state)
         {
            Trace trace{ "domain::delay::start"};

            //
            // Connect to domain
            //
            process::instance::connect( identification());

            message::pump( state);

         }

         bool operator < ( const State::Message& lhs, const State::Message& rhs)
         {
            return lhs.deadline < rhs.deadline;
         }

         void State::add( message::Request&& message)
         {
            Message delay;
            delay.destination = message.destination;
            delay.message = std::move( message.message);
            delay.deadline = platform::time::clock::type::now() + message.delay;

            m_messages.push_back( std::move( delay));
         }

         std::vector< State::Message> State::passed( common::platform::time::point::type time)
         {
            auto partition = range::partition( m_messages, [=]( const State::Message& m){
               return m.deadline > time;
            });

            std::vector< State::Message> result;

            range::move( std::get< 1>( partition), result);
            range::erase( m_messages, std::get< 1>( partition));

            return result;
         }

         common::platform::time::unit State::timeout() const
         {
            auto min = range::min( m_messages);

            if( min)
            {
               return std::chrono::duration_cast< common::platform::time::unit>(
                     min->deadline - platform::time::clock::type::now()
               );
            }
            return common::platform::time::unit::min();
         }

         namespace handle
         {
            struct Base
            {
               Base( State& state) : m_state( state) {}

            protected:
               State& m_state;
            };

            struct Request : Base
            {
               using Base::Base;

               void operator () ( message::Request& message)
               {
                  m_state.add( std::move( message));
               }
            };

            void timeout( State& state)
            {
               signal::thread::scope::Block block;

               for( auto&& message : state.passed())
               {
                  try
                  {
                     communication::ipc::outbound::Device ipc{ message.destination};

                     if( ! ipc.put( message.message, communication::ipc::policy::non::Blocking{}))
                     {
                        log << "failed to send delayed message to ipc: " << message.destination << " - action: try to resend in 500ms\n";

                        //
                        // Could not send... We set a new timeout in .5s
                        //
                        message::Request request;
                        request.destination = message.destination;
                        request.message = std::move( message.message);
                        request.delay = std::chrono::milliseconds{ 500};
                        state.add( std::move( request));

                     }
                  }
                  catch( const exception::system::communication::Unavailable&)
                  {
                     log << "failed to send delayed message to ipc: " << message.destination << " queue is unavailable - action: ignore\n";
                  }
               }
            }

            struct Timeout : Base
            {
               using Base::Base;

               void operator() ()
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::signal::Timeout&)
                  {
                     //
                     // Timeout has occurred, lets try to send the delayed messages
                     //
                     timeout( m_state);

                  }
               }
            };


         } // handle

         namespace message
         {
            void pump( State& state)
            {
               Trace trace{ "domain::delay::message::pump"};


               communication::ipc::Helper ipc{ handle::Timeout{ state}};


               auto handler = ipc.handler(
                  common::message::handle::ping(),
                  common::message::handle::Shutdown{},
                  handle::Request{ state}
               );

               while( true)
               {
                  //
                  // Set timeout, if any
                  //
                  signal::timer::set( state.timeout());

                  handler( ipc.blocking_next());
               }
            }
         } // message


         int main( int argc, char **argv)
         {

            try
            {
               Settings settings;
               {
                  casual::common::Arguments parser{{

                  }};
                  parser.parse( argc, argv);
               }

               start( std::move( settings));
            }
            catch( ...)
            {
               return casual::common::exception::handle();
            }
            return 0;
         }

      } // delay

   } // domain

} // casual


