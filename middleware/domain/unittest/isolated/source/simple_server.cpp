//!
//! casual 
//!

#include "common/communication/ipc.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

namespace casual
{
   namespace local
   {
      namespace
      {
         int main(int argc, char **argv)
         {
            try
            {
               common::process::instance::connect();

               auto handler = common::communication::ipc::inbound::device().handler(
                     common::message::handle::Ping{},
                     common::message::handle::Shutdown{}
               );

               common::message::dispatch::blocking::pump( handler, common::communication::ipc::inbound::device());
            }
            catch( ...)
            {
               return common::exception::handle();
            }
            return 0;
         }

      } // <unnamed>
   } // local

} // casual

int main(int argc, char **argv)
{
   return casual::local::main( argc, argv);
}

