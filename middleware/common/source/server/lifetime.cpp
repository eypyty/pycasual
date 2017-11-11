//!
//! casual
//!

#include "common/server/lifetime.h"
#include "common/communication/ipc.h"

#include "common/log.h"


namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace lifetime
         {

            namespace soft
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout)
               {
                  Trace trace{ "common::server::lifetime::soft::shutdown"};

                  log::debug << "servers: " << range::make( servers) << std::endl;

                  auto result = process::lifetime::ended();


                  std::vector< strong::process::id> requested;

                  for( auto& handle : servers)
                  {
                     message::shutdown::Request message;

                     try
                     {
                        if( communication::ipc::non::blocking::send( handle.queue, message))
                        {
                           requested.push_back( handle.pid);
                        }
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        //
                        // The server's queue is absent...
                        //
                     }

                  }

                  auto terminated = process::lifetime::wait( requested, timeout);

                  range::append( std::get< 0>( range::intersection( terminated, requested)), result);

                  log::debug << "soft off-line: " << range::make( result) << std::endl;

                  return result;

               }

            } // soft

            namespace hard
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout)
               {
                  Trace trace{ "common::server::lifetime::hard::shutdown"};

                  std::vector< strong::process::id> origin;

                  for( auto& handle: servers)
                  {
                     origin.push_back( handle.pid);
                  }

                  auto result = soft::shutdown( servers, timeout);


                  auto running = range::difference( origin, result);

                  log::debug << "still on-line: " << range::make( running) << std::endl;

                  range::append( process::lifetime::terminate( range::to_vector( running), timeout), result);

                  log::debug << "hard off-line: " << std::get< 0>( range::intersection( running, result)) << std::endl;

                  return result;

               }
            } // hard



         } // lifetime

      } // server

   } // common



} // casual
