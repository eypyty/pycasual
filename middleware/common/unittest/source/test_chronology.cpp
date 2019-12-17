//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/chronology.h"
#include "common/exception/system.h"


namespace casual
{
   namespace common
   {
      namespace chronology
      {
         TEST( common_chronology, from_string__invalid_unit)
         {
            EXPECT_THROW( from::string( "42ps"), exception::system::invalid::Argument);
         }

         TEST( common_chronology, from_string__ms)
         {
            EXPECT_TRUE( from::string( "42ms") == std::chrono::milliseconds( 42));
            EXPECT_TRUE( from::string( "0ms") == std::chrono::milliseconds::zero());
            EXPECT_TRUE( from::string( "ms") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__s)
         {
            EXPECT_TRUE( from::string( "42s") == std::chrono::seconds( 42));
            EXPECT_TRUE( from::string( "42") == std::chrono::seconds( 42));
            EXPECT_TRUE( from::string( "0s") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "s") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__min)
         {
            EXPECT_TRUE( from::string( "42min") == std::chrono::minutes( 42));
            EXPECT_TRUE( from::string( "0min") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "min") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__h)
         {
            EXPECT_TRUE( from::string( "42h") == std::chrono::hours( 42));
            EXPECT_TRUE( from::string( "0h") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "h") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__us)
         {
            EXPECT_TRUE( from::string( "42us") == std::chrono::microseconds( 42));
            EXPECT_TRUE( from::string( "0us") == std::chrono::seconds::zero());
            EXPECT_TRUE( from::string( "us") == std::chrono::milliseconds::zero());
         }

         TEST( common_chronology, from_string__h_min_s)
         {
            EXPECT_TRUE( from::string( "1h+2min+12s") == std::chrono::seconds( 3600 + 120 + 12));
            EXPECT_TRUE( from::string( "1h + 2min + 12s") == std::chrono::seconds( 3600 + 120 + 12));
         }



      } // chronology
   } // common
} // casual
