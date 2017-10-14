//!
//! casual
//!


#include "common/process.h"
#include "common/arguments.h"
#include "common/environment.h"



#include <string>
#include <vector>

#include <iostream>


namespace casual
{
   namespace admin
   {

      namespace dispatch
      {
         void execute( const std::string& command, const std::vector< std::string>& arguments, const std::string& subpath = "/internal/bin/")
         {
            static const auto path = common::environment::variable::get( "CASUAL_HOME");

            if( common::process::execute( path + subpath + command, arguments) != 0)
            {
               // TODO: throw?
            }
         }


         void domain( const std::vector< std::string>& arguments)
         {
            //common::directory::scope::Change change{ common::environment::string( "${CASUAL_DOMAIN_HOME}")};
            execute( "casual-domain-admin", arguments);
         }

         void service( const std::vector< std::string>& arguments)
         {
            //common::directory::scope::Change change{ common::environment::string( "${CASUAL_DOMAIN_HOME}")};
            execute( "casual-service-admin", arguments);
         }

         void queue( const std::vector< std::string>& arguments)
         {
            //common::directory::scope::Change change{ common::environment::string( "${CASUAL_DOMAIN_HOME}")};
            execute( "casual-queue-admin", arguments);
         }


         void transaction( const std::vector< std::string>& arguments)
         {
            //common::directory::scope::Change change{ common::environment::string( "${CASUAL_DOMAIN_HOME}")};
            execute( "casual-transaction-admin", arguments);
         }

         void gateway( const std::vector< std::string>& arguments)
         {
            //common::directory::scope::Change change{ common::environment::string( "${CASUAL_DOMAIN_HOME}")};
            execute( "casual-gateway-admin", arguments);
         }

         void call( const std::vector< std::string>& arguments)
         {
            execute( "casual-service-call", arguments, "/bin/");
         }

         void describe( const std::vector< std::string>& arguments)
         {
            execute( "casual-service-describe", arguments, "/bin/");
         }


      } // dispatch


      int main( int argc, char **argv)
      {
         try
         {

            common::Arguments arguments{ R"(
usage: 

casual [<category> [<category-specific-directives].. ]..

To get help for a specific category use: 
   casual <category> --help

The following categories are supported:   
  
)", { "help"},
            {
               common::argument::directive( { "domain" }, "domain related administration", &dispatch::domain),
               common::argument::directive( { "service" }, "service related administration", &dispatch::service),
               common::argument::directive( { "queue" }, "casual-queue related administration", &dispatch::queue),
               common::argument::directive( { "transaction" }, "transaction related administration", &dispatch::transaction),
               common::argument::directive( { "gateway" }, "gateway related administration", &dispatch::gateway),
               common::argument::directive( { "call" }, "generic service call", &dispatch::call),
               common::argument::directive( { "describe" }, "describes services", &dispatch::describe),
            }};

            arguments.parse( argc, argv);

         }
         catch( const common::argument::exception::Help&)
         {
         }
         catch( ...)
         {
            return common::exception::handle( std::cerr);
         }

         return 0;
      }

   } // admin

} // casual


int main( int argc, char **argv)
{
   return casual::admin::main( argc, argv);
}


