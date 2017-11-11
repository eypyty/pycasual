//!
//! casual
//!

#include "common/chronology.h"
#include "common/exception/system.h"
#include "common/environment.h"

#include <ctime>

#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>

namespace casual
{

namespace common
{

namespace chronology
{

   namespace internal
   {

      template<typename F>
      std::string format( const platform::time::point::type& time, F function)
      {
         if( time == platform::time::point::type::min())
         {
            return "0000-00-00T00:00:00.000";
         }

         //
         // to_time_t does not exist as a static member in common::clock_type
         //
         auto time_t = platform::time::clock::type::to_time_t( time);
         //const std::time_t seconds = std::chrono::duration_cast< std::chrono::seconds>( time.time_since_epoch()).count();

         struct tm time_parts;

         {
            //
            // Until we get a thread safe localtime_r we lock
            //
            std::unique_lock< std::mutex> lock{ environment::variable::mutex()};
            function( &time_t, &time_parts);
         }

         const auto ms = std::chrono::duration_cast< std::chrono::milliseconds>( time.time_since_epoch());

         std::ostringstream result;
         result << std::setfill( '0') <<
            std::setw( 4) << time_parts.tm_year + 1900 << '-' <<
            std::setw( 2) << time_parts.tm_mon + 1 << '-' <<
            std::setw( 2) << time_parts.tm_mday << 'T' <<
            std::setw( 2) << time_parts.tm_hour << ':' <<
            std::setw( 2) << time_parts.tm_min << ':' <<
            std::setw( 2) << time_parts.tm_sec << '.' <<
            std::setw( 3) << ms.count() % 1000;
         return result.str();
      }

   }


   std::string local( const platform::time::point::type& time)
   {
      return internal::format( time, &::localtime_r);
   }

   std::string local()
   {
      return local( platform::time::clock::type::now());
   }

   std::string universal()
   {
      return local( platform::time::clock::type::now());
   }

   std::string universal( const platform::time::point::type& time)
   {
      return internal::format( time, &gmtime_r);
   }

   namespace from
   {
      common::platform::time::unit string( const std::string& value)
      {
         using time_unit = common::platform::time::unit;

         auto last = std::find_if_not( std::begin( value), std::end( value), []( std::string::value_type value)
               {
                  return value >= '0' && value <= '9';
               });

         decltype( std::stoull( "")) count = 0;

         if( last != std::begin( value))
         {
            count = std::stoull( std::string{ std::begin( value), last});
         }

         const std::string unit{ last, std::end( value)};

         if( unit.empty() || unit == "s") return std::chrono::seconds( count);
         if( unit == "ms") return std::chrono::duration_cast< time_unit>( std::chrono::milliseconds( count));
         if( unit == "min") return std::chrono::minutes( count);
         if( unit == "us") return std::chrono::duration_cast< time_unit>( std::chrono::microseconds( count));
         if( unit == "h") return std::chrono::hours( count);
         if( unit == "d") return std::chrono::hours( count * 24);
         if( unit == "ns") return std::chrono::duration_cast< time_unit>( std::chrono::nanoseconds( count));


         throw exception::system::invalid::Argument{ "invalid time representation: " + value};
      }
   } // from

} // chronology

} // utility

} // casual
