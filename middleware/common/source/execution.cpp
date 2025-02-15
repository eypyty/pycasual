//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/execution.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/exception/handle.h"


namespace casual
{
   namespace common
   {
      namespace execution
      {
         namespace local
         {
            namespace
            {
               constexpr auto environment = "CASUAL_EXECUTION_ID";

               auto reset()
               {
                  auto id = uuid::make();
                  environment::variable::set( environment, uuid::string( id));
                  return id;
               }

               auto initialize()
               {
                  return environment::variable::get( environment, &reset);
               }

               execution::type& id()
               {
                  static execution::type id{ initialize()};
                  return id;
               }
            } // <unnamed>
         } // local

         void id( const type& id)
         {
            local::id() = id;
         }

         void reset()
         {
            local::id() = execution::type{ local::reset()};
         }

         const type& id()
         {
            return local::id();
         }

         namespace service
         {
            namespace local
            {
               namespace
               {
                  std::string& name()
                  {
                     static std::string name;
                     return name;
                  }
               } // <unnamed>
            } // local


            void name( const std::string& service)
            {
               local::name() = service;
            }

            const std::string& name()
            {
               return local::name();
            }

            void clear()
            {
               local::name().clear();
            }


            namespace parent
            {
               namespace local
               {
                  namespace
                  {
                     std::string& name()
                     {
                        static std::string name;
                        return name;
                     }
                  } // <unnamed>
               } // local

               void name( const std::string& service)
               {
                  local::name() = service;
               }

               const std::string& name()
               {
                  return local::name();
               }

               void clear()
               {
                  local::name().clear();
               }

            } // parent
         } // service

      } // execution
   } // common
} // casual


