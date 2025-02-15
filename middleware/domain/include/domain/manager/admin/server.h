//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/server/argument.h"

namespace casual
{
   namespace domain
   {

      namespace manager
      {
         struct State;

         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state = ".casual/domain/state";
                  namespace scale
                  {
                     constexpr auto aliases = ".casual/domain/scale/aliases";
                     // [[deprecated]] constexpr auto instances = ".casual/domain/scale/instances";
                  } // scale

                  namespace restart
                  {
                     constexpr auto aliases = ".casual/domain/restart/aliases";
                     constexpr auto groups = ".casual/domain/restart/groups";

                     // [[deprecated]] constexpr auto instances = ".casual/domain/restart/instances";
                  } // restart

                  constexpr auto shutdown = ".casual/domain/shutdown";

                  namespace configuration
                  {
                     constexpr auto get = ".casual/domain/configuration/get";
                     constexpr auto put = ".casual/domain/configuration/put";
                     constexpr auto post = ".casual/domain/configuration/post";
                  } // configuration

                  namespace environment
                  {
                     constexpr auto set = ".casual/domain/environment/set";
                  } // environment

               } // name
            } // service

            common::server::Arguments services( manager::State& state);


         } // admin
      } // manager
   } // admin

} // casual


