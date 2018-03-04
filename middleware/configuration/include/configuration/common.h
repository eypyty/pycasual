//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_COMMON_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace configuration
   {
      extern common::log::Stream log;

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
      };

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_COMMON_H_
