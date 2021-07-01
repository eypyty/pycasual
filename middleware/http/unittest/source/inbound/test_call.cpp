//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "http/inbound/call.h"


#include "domain/manager/unittest/process.h"

namespace casual
{
   using namespace common;

   namespace http::inbound
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   name: base

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
)";
               

            } // configuration

            auto domain()
            {
               return casual::domain::manager::unittest::Process{ { configuration::base}};
            }


            auto wait( call::Context context) -> decltype( context.receive())
            {
               auto count = 1000;

               while( count-- > 0)
               {
                  if( auto reply = context.receive())
                     return reply;
                  process::sleep( std::chrono::milliseconds{ 1});
               }
               return {};
            }

            namespace header
            {
               auto value( const std::vector< call::header::Field>& header, std::string_view key)
               {
                  if( auto found = algorithm::find( header, key))
                     return found->value;

                  common::code::raise::error( common::code::casual::invalid_argument, "unittest - failed to find key: ", key);
               }
            } // header


         } // <unnamed>
      } // local

      TEST( http_inbound_call, call_echo)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         constexpr std::string_view json = R"(
{
   "a" : 42  
}
)";
         auto request = [&json]()
         {
            call::Request result;
            result.service = "casual/example/echo";
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( json, result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         EXPECT_TRUE( reply.code == http::code::ok);
         EXPECT_TRUE( algorithm::equal( reply.payload.body, json));
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-type") == "application/json");
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == std::to_string( json.size()));

         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-code") == "OK");
         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-user-code") == "0");

      }

      TEST( http_inbound_call, call_non_exixting_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         constexpr std::string_view json = R"(
{}
)";
         auto request = [&json]()
         {
            call::Request result;
            result.service = "non-existent-service";
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( json, result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         EXPECT_TRUE( reply.code == http::code::not_found) << CASUAL_NAMED_VALUE( reply.code);

         // TODO: we don't set the content-type on no_entry, is this a problem?
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == "0");
      }

      TEST( http_inbound_call, call_rollback)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         constexpr std::string_view json = R"(
{
   "a": "b"
}
)";
         auto request = [&json]()
         {
            call::Request result;
            result.service = "casual/example/rollback";
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( json, result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         // TODO: is 500 really the best code?
         EXPECT_TRUE( reply.code == http::code::internal_server_error) << reply.code;
         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-code") == "TPESVCFAIL");

         EXPECT_TRUE( local::header::value( reply.payload.header, "content-type") == "application/json");
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == std::to_string( json.size()));

      }


   } // http::inbound
   
} // casual