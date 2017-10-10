//!
//! casual
//!



#include "common/arguments.h"
#include "common/process.h"
#include "common/exception/handle.h"

#include "common/environment.h"

#include "transaction/manager/manager.h"




int main( int argc, char** argv)
{

   try
   {
      casual::transaction::Settings settings;
      {
         casual::common::Arguments parser{ {
               casual::common::argument::directive( { "-db", "--database"}, "(depreciated) path to transaction database log", settings.log),
               casual::common::argument::directive( { "-l", "--transaction-log"}, "path to transaction database log", settings.log),
         }};

         parser.parse( argc, argv);
      }

      casual::transaction::Manager manager( settings);
      manager.start();

   }
   catch( ...)
   {
      return casual::common::exception::handle();

   }
   return 0;
}

