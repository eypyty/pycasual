//! 
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/administration/cli.h"

#include "common/unittest.h"

#include "common/argument.h"
#include "common/terminal.h"
#include "common/exception/handle.h"

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace documentation
      {
         namespace local
         {
            namespace
            {

               template< typename H, typename F>
               void generate( 
                  const std::vector< std::string>& options, 
                  const std::string& path,
                  H&& header,
                  F&& footer)
               {
                  // make sure we don't use colors...
                  terminal::output::directive().plain();

                  // make sure we create directories if not present
                  directory::create( directory::name::base( path));
                  std::ofstream out{ path};
                  header( out);

                  auto restore = unittest::capture::standard::out( out);
      
                  CLI cli;
                  cli.parser()( options);
               
                  footer( out);
               }

               void generate( const std::string& root)
               {
                  auto footer = []( auto& out)
                  {
                     out << "```\n";
                  };

                  auto generate_option = [&root, &footer]( auto option)
                  {
                     auto header = [option]( auto& out)
                     {
                        out << "# casual " << option << R"(

```shell
)" << "host# casual --help " << option << "\n\n";
                     };

                     generate( 
                        { "--help", option}, 
                        string::compose( root, "/cli/", option, ".operation.md"),
                        header,
                        footer);
                  };

                  generate_option( "domain");
                  generate_option( "service");
                  generate_option( "transaction");
                  generate_option( "queue");
                  generate_option( "gateway");
                  generate_option( "discovery");
                  generate_option( "buffer");
                  generate_option( "describe");
                  generate_option( "call");

                  // the casual cli
                  {

                     auto header = []( auto& out)
                     {
                        out << R"(# casual

```shell
)" << "host# casual --help " << "\n\n";
                     };

                     generate( 
                        { "--help"}, 
                        string::compose( root, "/cli/", "casual.operation.md"),
                        header,
                        footer);
                  }
                 
               }

               
               void main( int argc, char** argv)
               {
                  std::string root;

                  argument::Parse{ "generate cli documentation",
                     argument::Option{ std::tie( root), { "--root"}, "root for the generated markdown files"}
                  }( argc, argv);

                  generate( root);

               }
            } // <unnamed>
         } // local


         
      } // documentation
   } // administration
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::administration::documentation::local::main( argc, argv);
   });
}