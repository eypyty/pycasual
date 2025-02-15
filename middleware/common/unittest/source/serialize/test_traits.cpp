//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/traits.h"
#include "common/serialize/macro.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         

         TEST( casual_serialize_traits, has_serialize__false)
         {
            common::unittest::Trace trace;

            struct Value
            {
            };

            EXPECT_FALSE( ( traits::has::serialize_v< Value, long>));
         }

         namespace local
         {
            namespace
            {
               struct Value
               {
                  CASUAL_CONST_CORRECT_SERIALIZE()
               };
            } // <unnamed>
         } // local

         TEST( casual_serialize_traits, has_serialize__true)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( ( traits::has::serialize_v< local::Value, long>));
         }


         namespace local
         {
            namespace
            {
               namespace archive
               {
                  struct Named
                  {
                     inline constexpr static auto archive_type() { return serialize::archive::Type::static_need_named;}
                  };
                  struct Order
                  {
                     inline constexpr static auto archive_type() { return serialize::archive::Type::static_order_type;}
                  };
               } // archive
               
            } // <unnamed>
         } // local

         TEST( casual_serialize_traits, need_named__true)
         {

            EXPECT_TRUE( ( traits::need::named_v< local::archive::Named>));
         }

         TEST( casual_serialize_traits, need_named__false)
         {
            EXPECT_TRUE( ( ! traits::need::named_v< local::archive::Order>));
         }
      } // serialize
   } // common
} // casual