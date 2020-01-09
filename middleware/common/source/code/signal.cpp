//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/signal.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace local
         {
            namespace
            {
               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "signal";
                  }

                  std::string message( int code) const override
                  {
                     switch( static_cast< code::signal>( code))
                     {
                        case signal::none : return "none";
                        case signal::alarm: return "alarm";
                        case signal::child: return "child";
                        case signal::interrupt: return "interupt";
                        case signal::kill: return "kill";
                        case signal::pipe: return "pipe";
                        case signal::quit: return "quit";
                        case signal::terminate: return "terminate";
                        case signal::user: return "user";
                        case signal::hangup: return "hangup";
                     }
                     return "unknown";
                  }
               };

               const Category category{};

            } // <unnamed>
         } // local

         std::error_code make_error_code( signal code)
         {
            return { static_cast< int>( code), local::category};
         }

         common::log::Stream& stream( code::signal code)
         {
            switch( code)
            {
               // debug
               //case casual::ok: return common::log::debug;

               // rest is errors
               default: return common::log::category::error;
            }
         }
      } // code
   } // common
} // casual
