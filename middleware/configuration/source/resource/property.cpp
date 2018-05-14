//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/resource/property.h"

#include "serviceframework/archive/create.h"


#include "common/environment.h"
#include "common/exception/system.h"


namespace casual
{
   namespace configuration
   {
      namespace resource
      {
         Property::Property() = default;
         Property::Property( std::function< void(Property&)> foreign) { foreign( *this);}

         namespace property
         {

            std::vector< Property> get( const std::string& name)
            {
               std::vector< Property> resources;

               //
               // Create the reader and deserialize configuration
               //
               common::file::Input file{ name};
               auto reader = serviceframework::archive::create::reader::consumed::from( file.extension(), file);

               reader >> CASUAL_MAKE_NVP( resources);
               reader.validate();

               //
               // Make sure we've got valid configuration
               //
               validate( resources);

               return resources;
            }

            namespace local
            {
               namespace
               {
                  std::string file()
                  {
                     auto file = common::environment::variable::get( "CASUAL_RESOURCE_CONFIGURATION_FILE", "");

                     if( file.empty())
                     {
                        return common::file::find(
                              common::environment::directory::casual() + "/configuration",
                              std::regex( "resources.(yaml|xml|json|ini)" ));
                     }

                     return file;
                  }
               } // <unnamed>
            } // local

            std::vector< Property> get()
            {
               //
               // Try to find configuration file
               //
               const auto file = local::file();

               if( ! file.empty())
               {
                  return get( file);
               }
               else
               {
                  throw common::exception::system::invalid::File( common::string::compose( "could not find resource configuration file",
                        " path: ", common::environment::directory::casual(),  "/configuration",
                        " name: ", "resources.(yaml|json|xml|..."));
               }
            }


         } // property


         void validate( const std::vector< Property>& properties)
         {

         }

      } // resource
   } // config
} // casual

