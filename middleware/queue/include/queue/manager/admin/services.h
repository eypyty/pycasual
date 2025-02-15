//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state = ".casual/queue/state";
                  constexpr auto restore = ".casual/queue/restore";
                  constexpr auto clear = ".casual/queue/clear";

                  namespace messages
                  {
                     constexpr auto list = ".casual/queue/messages/list";
                     constexpr auto remove = ".casual/queue/messages/remove";
                     
                  } // messages

                  namespace metric
                  {
                     constexpr auto reset = ".casual/queue/metric/reset";
                  } // metric 

                  namespace forward
                  {
                     namespace scale
                     {
                        constexpr auto aliases = ".casual/queue/forward/scale/aliases";
                     } // scale
                     
                  } // forward
                  
               } // name

            } // service

         } // admin
      } // manager
   } // queue

} // casual


