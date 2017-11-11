//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_TRAFFIC_INCLUDE_TRAFFIC_COMMON_H_
#define CASUAL_MIDDLEWARE_TRAFFIC_INCLUDE_TRAFFIC_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"


namespace casual
{
   namespace event
   {
      extern common::log::Stream log;

      namespace verbose
      {
         extern common::log::Stream log;
      } // verbose

      namespace trace
      {
         extern common::log::Stream log;
      } // verbose

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

   } // traffic


} // casual

#endif // CASUAL_MIDDLEWARE_TRAFFIC_INCLUDE_TRAFFIC_COMMON_H_
