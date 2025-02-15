//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "test/unittest/gateway.h"

#include "domain/manager/unittest/process.h"
#include "domain/manager/admin/cli.h"

#include "common/communication/instance.h"
#include "serviceframework/service/protocol/call.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"
#include "service/unittest/utility.h"

#include "transaction/unittest/utility.h"

#include "queue/api/queue.h"

#include "casual/xatmi.h"


#include <regex>

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
system:
   resources:
      -  key: rm-mockup
         server: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm

domain:
   transaction:
      resources:
         -  name: example-resource-server
            key: rm-mockup
            openinfo: foo
            instances: 1

   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";
               
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::manager::unittest::process( configuration::base, std::forward< C>( configurations)...);
            }


            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               common::unittest::random::range( range::make( buffer, size));
               return buffer;
            }

            inline auto call( std::string_view service, int code)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               if( tpcall( service.data(), buffer, 128, &buffer, &len, 0) == -1)
                  EXPECT_TRUE( tperrno == code) << "tpcall - expected: " << tperrnostring( code) << " got: " << tperrnostring( tperrno);
               else
                  EXPECT_TRUE( code == 0) << "tpcall - expected: " << tperrnostring( code) << " got: " << tperrnostring( 0);

               tpfree( buffer);
            };

            auto call( std::string_view service)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               tpcall( service.data(), buffer, 128, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               return memory::guard( buffer, &tpfree);
            };


            auto call( std::string_view service, const platform::binary::type& binary)
            {
               auto buffer = local::allocate( binary.size());
               assert( buffer);
               algorithm::copy( binary, buffer);

               auto len = tptypes( buffer, nullptr, nullptr);

               EXPECT_TRUE( tpcall( service.data(), buffer, len, &buffer, &len, 0) != -1) << "tperrno: " << tperrnostring( tperrno);

               return memory::guard( buffer, &tpfree);
            };

            auto acall( std::string_view service, const platform::binary::type& binary)
            {
               auto buffer = memory::guard( local::allocate( binary.size()), &tpfree);
               assert( buffer);
               algorithm::copy( binary, buffer.get());

               auto len = tptypes( buffer.get(), nullptr, nullptr);

               auto result = tpacall( service.data(), buffer.get(), len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               return result;
            };

            auto receive( int descriptor)
            {
               auto buffer = local::allocate( 128);
               assert( buffer);

               auto len = tptypes( buffer, nullptr, nullptr);

               tpgetrply( &descriptor, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               platform::binary::type result;
               algorithm::copy( range::make( buffer, len), std::back_inserter( result));
               tpfree( buffer);

               return result;
            };

            template< typename B>
            auto size( B&& buffer)
            {
               return tptypes( buffer.get(), nullptr, nullptr);
            }

            template< typename T>
            void sink( T&& value)
            {
               auto sinked = std::move( value);
            }

            namespace example
            {
               auto domain( std::string name, std::string port, std::string_view extended = {})
               {
                  constexpr auto template_config = R"(
domain: 
   name: NAME

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:PORT
)";               
                  auto replace = []( auto text, auto pattern, auto value)
                  {
                     return std::regex_replace( text, std::regex{ pattern}, value); 
                  };

                  if( extended.empty())
                     return local::domain( replace( replace( template_config, "NAME", name), "PORT", port));
                  else
                     return local::domain( replace( replace( template_config, "NAME", name), "PORT", port), extended);
               };

               namespace reverse
               {
                  auto domain( std::string name, std::string port, std::string_view extended = {})
                  {
                     constexpr auto template_config = R"(
domain: 
   name: NAME

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:PORT
)";               
                     auto replace = []( auto text, auto pattern, auto value)
                     {
                        return std::regex_replace( text, std::regex{ pattern}, value); 
                     };

                     return local::domain( replace( replace( template_config, "NAME", name), "PORT", port));
                  }
                  
               } // reverse
            } // example


            namespace lookup::domain
            {

               auto name( std::string_view name)
               {
                  return local::call( "casual/example/domain/name").get() == name;
               }

               auto name( std::string_view name, platform::size::type count)
               {
                  while( count-- > 0)
                  {
                     if( domain::name( name))
                        return true;
                     else
                        process::sleep( std::chrono::milliseconds{ 1});
                  }
                  return false;
               }
            } // lookup::domain

         } // <unnamed>
      } // local



      
      TEST( test_domain_gateway, queue_service_forward___from_A_to_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:6669
)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
      
   queue:
      groups:
         -  alias: a
            queuebase: ':memory:'
            queues:
               -  name: a1
               -  name: a2
      forward:
         groups:
            -  services:
                  -  source: a1
                     instances: 1
                     target:
                        service: casual/example/echo
                     reply:
                        queue: a2
   
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:6669
)";

      
         // startup the domain that will try to do stuff
         auto b = local::domain( B);
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());

         auto payload = common::unittest::random::binary( 1024);
         {
            queue::Message message;
            message.payload.type = common::buffer::type::json();
            message.payload.data = payload;
            queue::enqueue( "a1", message);
         }

         // startup the domain that has the service
         auto a = local::domain( A);
         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());

         // activate the b domain, so we can try to dequeue
         b.activate();

         {
            auto message = queue::blocking::dequeue( "a2");

            EXPECT_TRUE( message.payload.type == common::buffer::type::json());
            EXPECT_TRUE( message.payload.data == payload);
         }
         
      }

      TEST( test_domain_gateway, domain_A_to_B_to_C__expect_B_hops_1___C_hops_2)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto c = local::example::domain( "C", "7002");
         auto b = local::domain( R"(
domain: 
   name: B

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
                  discovery:
                     forward: true
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7002

)"); 
         
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         auto a = local::domain( R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");


         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // call the two domain services to get discovery
         local::call( "casual/example/domain/echo/B", 0);
         local::call( "casual/example/domain/echo/C", 0);

         // check the hops...
         auto state = casual::service::unittest::state();

         {
            auto found = algorithm::find( state.services, "casual/example/domain/echo/B");
            ASSERT_TRUE( found);
            EXPECT_TRUE( found->instances.sequential.empty());
            EXPECT_TRUE( found->instances.concurrent.at( 0).hops == 1) << CASUAL_NAMED_VALUE( found->instances.concurrent.at( 0));
         }

         {
            auto found = algorithm::find( state.services, "casual/example/domain/echo/C");
            ASSERT_TRUE( found);
            EXPECT_TRUE( found->instances.sequential.empty());
            EXPECT_TRUE( found->instances.concurrent.at( 0).hops == 2) << CASUAL_NAMED_VALUE( found->instances.concurrent.at( 0));
         }
         
      }

      TEST( test_domain_gateway, domain_A_to_B_C_D__D_to__E__expect_call_to_only_B_C___shutdown_B_C__exepct_call_to_D_forward_to_E)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto e = local::example::domain( "E", "7001");
         auto d = local::domain( R"(
domain: 
   name: D
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7002
                  discovery:
                     forward: true
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7001

)"); 
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         auto c = local::example::domain( "C", "7003");
         auto b = local::example::domain( "B", "7004");

         
         
         
         auto a = local::domain( R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
                  -  address: 127.0.0.1:7003
                  -  address: 127.0.0.1:7004
)");


         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we expect to reach only  B C 
         algorithm::for_n< 10>([]()
         {
            auto buffer = local::call( "casual/example/domain/name");
            EXPECT_TRUE( algorithm::compare::any( std::string_view{ buffer.get()}, "B", "C"));
         });

         local::sink( std::move( c));
         local::sink( std::move( b));

         // wait until we only have one outbound connected (D)
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected( 1));

         // we expect all calls to reach E (via D)
         algorithm::for_n< 10>([]()
         {
            auto buffer = local::call( "casual/example/domain/name");
            EXPECT_TRUE( std::string_view{ buffer.get()} == "E");
         });

      }

      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::lookup::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            ASSERT_TRUE( local::lookup::domain::name( "B"));
         });
      }


      TEST( BACKWARD_test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B)
      {
         // same test as above but with the deprecated configuration

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      connections:
         -  address: 127.0.0.1:7001
            services:
               -  casual/example/domain/name

         -  address: 127.0.0.1:7002
            services:
               -  casual/example/domain/name
               
         -  address: 127.0.0.1:7003
            services:
               -  casual/example/domain/name
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::lookup::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "B"));
         });
      }


      TEST( test_domain_gateway, domain_A_to_B___echo_send_large_message)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/echo
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( 3 * platform::ipc::transport::size);

         algorithm::for_n< 10>( [&binary]()
         {
            auto buffer = local::call( "casual/example/echo", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));

            auto range = range::make( buffer.get(), size);
            EXPECT_TRUE( algorithm::equal( range, binary)) 
               << "buffer: " << transcode::hex::encode( range)
               << "\nbinary: " << transcode::hex::encode( binary);
         });
      }

      TEST( test_domain_gateway, domain_A_to_B___echo_send_message_in_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/echo
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 10>( [&binary]()
         {
            auto buffer = local::call( "casual/example/echo", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( test_domain_gateway, domain_A_to_B_C___call_A_B__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                  -  address: 127.0.0.1:7002
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/B", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B__to__C_D___call_C_D__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     discovery:
                        forward: true

      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
                     services: [ casual/example/domain/echo/C]
                  -  address: 127.0.0.1:7003
                     services: [ casual/example/domain/echo/D]
)";


         auto b = local::domain(  B);
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         auto a = local::domain(  A);
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B__reverse_to__C_D___call_C_D__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});


         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     discovery:
                        forward: true

      reverse:
         outbound:
            groups:
               -  connections:
                     -  address: 127.0.0.1:7002
                        services:
                           -  casual/example/domain/echo/C
                           -  casual/example/domain/echo/D
)";


         auto b = local::domain(  B);

         auto c = local::example::reverse::domain( "C", "7002");
         auto d = local::example::reverse::domain( "D", "7002");


         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         auto a = local::domain(  A);
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( test_domain_gateway, domain_A_to_B__to__C_D___call_C_D__from_A__call_D_from_C__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto c = local::example::domain( "C", "7002", R"(
domain:
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

)");
         auto d = local::example::domain( "D", "7003");

         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     discovery:
                        forward: true

      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
                     services: [ casual/example/domain/echo/C]
                  -  address: 127.0.0.1:7003
                     services: [ casual/example/domain/echo/D]
)";


         auto b = local::domain(  B);
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         auto a = local::domain(  A);
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         {
            // 'enter' domain C
            c.activate();

            // the outbound in C might not have been able to connect to A yet, wait for it.
            test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

            auto buffer = local::call( "casual/example/domain/echo/D", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));
         }

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B___tpacall__sleep___shutdown_B___expect_reply___call_sleep_again__expect__TPENOENT)
      {
         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 100ms]
         instances: 5
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
)");



         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/sleep
         routes: [ sleepy]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( 2048);

         auto descriptors = algorithm::generate_n< 7>( [&binary]()
         {
            return local::acall( "sleepy", binary);
         });

         // all calls will take at least 100ms, we need to 'make sure' that the calls are 
         // in-flight. We'll sleep for 50ms to mitigate that calls did not make it to the service
         // before scaling down. 50ms should be enough for even the slowest of systems.
         // another way is to accept TPENOENT replies, but then we wouldn't really make sure
         // we test the thing we want to test.
         process::sleep( std::chrono::milliseconds{ 50});


         // shutdown B 
         local::sink( std::move( b));
         // <-- b is down.


         // receive the 7 calls
         for( auto descriptor : descriptors)
         {
            auto result = local::receive( descriptor);
            EXPECT_TRUE( result == binary);
         }

         // expect new calls to get TPENOENT
         local::call( "sleepy", TPENOENT);
      }


      TEST( test_domain_gateway, domain_A_to_B___tpacall__sleep___scale_B_down__expect__TPENOENT)
      {
         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 100ms]
         instances: 5
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
)");



         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         const auto binary = common::unittest::random::binary( 2048);

         auto descriptors = algorithm::generate_n< 5>( [&binary]()
         {
            return local::acall( "casual/example/sleep", binary);
         });

         // all calls will take at least 100ms, we need to 'make sure' that the calls are 
         // in-flight. We'll sleep for 50ms to mitigate that calls did not make it to the service
         // before scaling down. 50ms should be enough for even the slowest of systems.
         // another way is to accept TPENOENT replies, but then we wouldn't really make sure
         // we test the thing we want to test.
         process::sleep( std::chrono::milliseconds{ 50});
         
         {
            b.activate();

            casual::domain::manager::admin::cli cli;
            common::argument::Parse parse{ "", cli.options()};
            parse( { "domain", "--scale-instances", "casual-example-server", "0"});
         }

         a.activate();

         // receive the 10 calls
         for( auto descriptor : descriptors)
         {
            auto result = local::receive( descriptor);
            EXPECT_TRUE( result == binary);
         }

         // expect new calls to get TPENOENT
         local::call( "casual/example/sleep", TPENOENT);
      }

      TEST( test_domain_gateway, domain_A_to_B__B_to_A__A_knows_of_echo_in_B__scale_echo_down__call_echo_from_B____expect_TPNOENT)
      {
         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         auto b = local::domain(  R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 100ms]
         instances: 1
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7001

      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
)");



         auto a = local::domain( R"(
domain: 
   name: A

   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services: [ casual/example/sleep]
)");


         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         // jump in to B
         b.activate();

         casual::domain::manager::admin::cli cli;
         common::argument::Parse parse{ "", cli.options()};
         parse( { "domain", "--scale-instances", "casual-example-server", "0"});


         // calls to get TPENOENT
         local::call( "casual/example/sleep", TPENOENT);
      }


      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B__shutdown_B__expect_C__shutdown__C__expect_D)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::lookup::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "B"));
         });

         // shutdown B
         local::sink( std::move( b));

         // we expect to always get to C
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "C"));
         });

         // shutdown C
         local::sink( std::move( c));

         // we expect to always get to D ( the only on left...)
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "D"));
         });
      }

      TEST( test_domain_gateway, domain_A_to__B_C__outbound_separated_groups___expect_prio_B__shutdown_B__expect_C__boot_B__expect_B)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
)";


         auto a = local::domain(  A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we might get to C until all services are advertised.
         EXPECT_TRUE( local::lookup::domain::name( "B", 1000));

         // after both ar up, we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "B"));
         });

         // shutdown B
         local::sink( std::move( b));

         // we expect to always get to C
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "C"));
         });

         // boot B
         b = local::example::domain( "B", "7001");

         // we need to activate a, otherwise b has 'the control'
         a.activate();

         // we need to wait for all to be connected...
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         // we expect to get to B agin
         EXPECT_TRUE( local::lookup::domain::name( "B", 1000));

         // we expect to always get to B, again
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::lookup::domain::name( "B"));
         });
      }

      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_same_group___expect_round_robin_between_A_B_C)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";

         
         auto a = local::domain(  A);

         auto ready_predicate = []( auto& state)
         {
            if( auto found = algorithm::find( state.services, "casual/example/domain/name"))
               return found->connections.size() == 3;

            return false;
         };

         test::unittest::gateway::state::until( ready_predicate);

         std::map< std::string, int> domains;

         algorithm::for_n< 9>( [&domains]() mutable
         {
            auto buffer = local::call( "casual/example/domain/name");
            domains[ buffer.get()]++;
         });

         // expect even distribution
         EXPECT_TRUE( domains[ "B"] == 3);
         EXPECT_TRUE( domains[ "C"] == 3);
         EXPECT_TRUE( domains[ "D"] == 3);
      }

      TEST( test_domain_gateway, domains_A_B__B_has_echo__call_echo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( B);
         auto a = local::domain( A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         local::call( "casual/example/echo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "casual/example/echo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = local::domain( B);
         a.activate();

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");

         // expect to discover echo...
         local::call( "casual/example/echo", 0);
      }

      TEST( test_domain_gateway, domains_A_B__B_has_route_b_to_echo____A_has_route_a_to_b___call_route_a_from_A___expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: b
         routes: [ a]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   services:
      -  name: casual/example/echo
         routes: [ b]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( B);
         auto a = local::domain( A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         local::call( "a", 0);
         
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( B);
         auto a = local::domain( A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = local::domain( B);
         a.activate();

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_and_bar_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo, bar]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( B);
         auto a = local::domain( A);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = local::domain( B);
         a.activate();

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);

      }

      
      TEST( test_domain_gateway, call_A_B_from_C__via_GW__within_transaction__a_lot___expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto GW = R"(
domain: 
   name: GW

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770
               -  address: 127.0.0.1:7771
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772
                  discovery:
                     forward: true

)";


         constexpr auto C = R"(
domain: 
   name: C

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7772

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( B);
         auto a = local::domain( A);
         auto gw = local::domain( GW);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         auto c = local::domain( C);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         auto call_A_and_B = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/domain/echo/A", TPOK);
               local::call( "casual/example/resource/domain/echo/B", TPOK);
            });
         };

         constexpr auto transaction_count = 10;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_A_and_B]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_A_and_B( call_count);
            ASSERT_TRUE( tx_commit() == TX_OK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in c
         check_transaction_state();

         // check in gateway
         gw.activate();
         check_transaction_state();

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }        
      }

      TEST( test_domain_gateway, call_A_B_C_X_from_X__via_GW__within_transaction__one_call_tree_that_goes_all_over_the_place___expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/B, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/A, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/nested/calls/A, casual/example/resource/nested/calls/B, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772

)";

         constexpr auto GW = R"(
domain: 
   name: GW

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770
               -  address: 127.0.0.1:7771
               -  address: 127.0.0.1:7772
               -  address: 127.0.0.1:7773
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7779
                  discovery:
                     forward: true

)";


         constexpr auto X = R"(
domain: 
   name: X
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         instances: 3
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7773

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto a = local::domain( A);
         auto b = local::domain( B);
         auto c = local::domain( C);
         auto x = local::domain( X);
          
         auto gw = local::domain( GW);

         // everything is booted, we need to 'wait' until all domains got outbound connections
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         a.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         b.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         c.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         x.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we're in X at the moment

         auto call_A_and_B = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/nested/calls/C", TPOK);
            });
         };

         constexpr auto transaction_count = 5;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_A_and_B]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_A_and_B( call_count);
            ASSERT_TRUE( tx_commit() == TX_OK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in x
         {  
            // we're in X at the moment 
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }


         // check in gateway
         {
            gw.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == 0) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            c.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }  
      }


      TEST( test_domain_gateway, call_A_B_C_from_X__via_GW__A_B_C_connects_directly_to_X_within_transaction__one_call_tree_that_goes_all_over_the_place___expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/B, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/A, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/nested/calls/A, casual/example/resource/nested/calls/B, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772

)";

         constexpr auto GW = R"(
domain: 
   name: GW

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770 # A 
               -  address: 127.0.0.1:7771 # B
               -  address: 127.0.0.1:7772 # C
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7779
                  discovery:
                     forward: true

)";


         constexpr auto X = R"(
domain: 
   name: X
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         instances: 3
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7773

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto a = local::domain( A);
         auto b = local::domain( B);
         auto c = local::domain( C);
         auto x = local::domain( X);
          
         auto gw = local::domain( GW);

         // everything is booted, we need to 'wait' until all domains got outbound connections
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         a.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         b.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         c.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         x.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we're in X at the moment

         auto call_A_and_B = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/nested/calls/C", TPOK);
            });
         };

         constexpr auto transaction_count = 5;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_A_and_B]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_A_and_B( call_count);
            ASSERT_TRUE( tx_commit() == TX_OK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in x
         {  
            // we're in X at the moment 
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }


         // check in gateway
         {
            gw.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == 0) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            c.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }  
      }


      TEST( test_domain_gateway, call_A_B_C_from_X__via_GW__A_B_C_connects_directly_to_X_within_transaction__resource_report_XAER_RMERR_on_prepare_in_domain_A__expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   transaction:
      resources:
         -  name: example-resource-server
            key: rm-mockup
            # XAER_RMERR   -3 
            openinfo: --prepare -3
            instances: 1
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/B, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/A, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/nested/calls/A, casual/example/resource/nested/calls/B, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772

)";

         constexpr auto GW = R"(
domain: 
   name: GW

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770 # A 
               -  address: 127.0.0.1:7771 # B
               -  address: 127.0.0.1:7772 # C
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7779
                  discovery:
                     forward: true

)";


         constexpr auto X = R"(
domain: 
   name: X
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         instances: 3
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7773

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto a = local::domain( A);
         auto b = local::domain( B);
         auto c = local::domain( C);
         auto x = local::domain( X);
          
         auto gw = local::domain( GW);

         // everything is booted, we need to 'wait' until all domains got outbound connections
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         a.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         b.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         c.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         x.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we're in X at the moment

         auto call_A_and_B = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/nested/calls/C", TPOK);
            });
         };

         constexpr auto transaction_count = 5;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_A_and_B]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_A_and_B( call_count);
            ASSERT_TRUE( tx_commit() == TX_ROLLBACK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in x
         {  
            // we're in X at the moment 
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }


         // check in gateway
         {
            gw.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == 0) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            c.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2 * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }  
      }


      TEST( test_domain_gateway, call_A_B_C_from_X__via_GW__A_B_C_connects_directly_to_X_within_transaction__resource_report_XAER_RMFAIL_on_start_end_in_domain_A__expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   transaction:
      resources:
         -  name: example-resource-server
            key: rm-mockup
            # #define XAER_RMFAIL  -7    /* resource manager unavailable */
            openinfo: --start -7 --end -7
            instances: 1
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/B, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/A, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto C = R"(
domain: 
   name: C

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/nested/calls/A, casual/example/resource/nested/calls/B, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772

)";

         constexpr auto GW = R"(
domain: 
   name: GW

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770 # A 
               -  address: 127.0.0.1:7771 # B
               -  address: 127.0.0.1:7772 # C
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7779
                  discovery:
                     forward: true

)";


         constexpr auto X = R"(
domain: 
   name: X
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         instances: 3
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7773

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto a = local::domain( A);
         auto b = local::domain( B);
         auto c = local::domain( C);
         auto x = local::domain( X);
          
         auto gw = local::domain( GW);

         // everything is booted, we need to 'wait' until all domains got outbound connections
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         a.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         b.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         c.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         x.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we're in X at the moment

         auto call_C = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/nested/calls/C", TPOK);
            });
         };

         constexpr auto transaction_count = 1;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_C]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_C( call_count);
            ASSERT_EQ( tx_rollback(), TX_OK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in x
         {  
            // we're in X at the moment 
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }


         // check in gateway
         {
            gw.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == 0) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 1) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            c.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }  
      }

      TEST( test_domain_gateway, call_A_B_C_from_X__via_GW__A_B_C_connects_directly_to_X_within_transaction___rollback___expect_correct_transaction_state)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/B, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/domain/echo/A, casual/example/resource/domain/echo/C, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7771

)";

         constexpr auto C = R"(
domain: 
   name: C
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         arguments: [ --nested-calls, casual/example/resource/nested/calls/A, casual/example/resource/nested/calls/B, casual/example/resource/domain/echo/X]
         instances: 4
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779  # GW
               -  address: 127.0.0.1:7773  # X
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7772

)";

         constexpr auto GW = R"(
domain: 
   name: GW
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7770 # A 
               -  address: 127.0.0.1:7771 # B
               -  address: 127.0.0.1:7772 # C
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7779
                  discovery:
                     forward: true

)";


         constexpr auto X = R"(
domain: 
   name: X
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server"
         memberships: [ user]
         instances: 3
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7779
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7773

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto a = local::domain( A);
         auto b = local::domain( B);
         auto c = local::domain( C);
         auto x = local::domain( X);
          
         auto gw = local::domain( GW);

         // everything is booted, we need to 'wait' until all domains got outbound connections
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());
         
         a.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         b.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         c.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         x.activate();
         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         // we're in X at the moment

         auto call_C = []( auto count)
         {
            algorithm::for_n( count, []()
            {
               local::call( "casual/example/resource/nested/calls/C", TPOK);
            });
         };

         constexpr auto transaction_count = 1;
         constexpr auto call_count = 1;

         algorithm::for_n( transaction_count, [call_C]()
         {
            ASSERT_TRUE( tx_begin() == TX_OK);
            call_C( call_count);
            ASSERT_EQ( tx_rollback(), TX_OK);
         });

         auto check_transaction_state = []( )
         {
            auto state = casual::transaction::unittest::state();

            EXPECT_TRUE( state.pending.persistent.replies.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.pending.requests.empty()) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

            return state;
         };

         // check in x
         {  
            // we're in X at the moment 
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }


         // check in gateway
         {
            gw.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, prepare and commit.
            EXPECT_TRUE( instance.metrics.resource.count == 0) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            a.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            b.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count  * 2) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }

         {
            c.activate();
            auto state = check_transaction_state();
            ASSERT_TRUE( state.resources.size() == 1);
            auto& resource = state.resources[ 0];
            ASSERT_TRUE( resource.instances.size() == 1);
            auto& instance = resource.instances[ 0];
            // distributed transactions, only rollback
            EXPECT_TRUE( instance.metrics.resource.count == transaction_count * 3) << CASUAL_NAMED_VALUE( instance.metrics.resource);
         }  
      }

      //! put in this TU to enable all helpers to check state
      TEST( test_domain_assassinate, interdomain_call__timeout_1ms__call)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A
   
   service:
      execution:
         timeout:
            duration: 2ms
            contract: kill
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 1s]
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7000
   

)";

         constexpr auto B = R"(
domain: 
   name: B

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7000
   
)";

         auto a = local::domain( A);
         auto b = local::domain( B);

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         auto start = platform::time::clock::type::now();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/sleep", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 2});

         tpfree( buffer);

      }

   } // test::domain
} // casual
