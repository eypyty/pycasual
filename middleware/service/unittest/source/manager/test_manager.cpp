//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"
#include "service/forward/instance.h"
#include "service/unittest/utility.h"

#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/service.h"

#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/algorithm/container.h"

#include "common/code/xatmi.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include "domain/discovery/api.h"
#include "domain/configuration/fetch.h"
#include "domain/manager/unittest/process.h"
#include "domain/manager/unittest/configuration.h"

#include "configuration/model/load.h"
#include "configuration/model/transform.h"

namespace casual
{
   namespace service
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& inbound() { return common::communication::ipc::inbound::device();}
            } // ipc

            namespace configuration
            {
               constexpr auto base = R"(
domain:
   name: service-domain

   servers:
      - path: bin/casual-service-manager
)";


               template< typename... C>
               auto load( C&&... contents)
               {
                  auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);

                  auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

                  return casual::configuration::model::load( common::algorithm::transform( files, get_path));
               }



            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return domain::manager::unittest::process( configuration::base, std::forward< C>( configurations)...);
            }

            namespace call
            {
               auto state()
               {
                  serviceframework::service::protocol::binary::Call call;

                  auto reply = call( manager::admin::service::name::state());

                  manager::admin::model::State serviceReply;

                  reply >> CASUAL_NAMED_VALUE( serviceReply);

                  return serviceReply;
               }

            } // call

            namespace service
            {
               const manager::admin::model::Service* find( const manager::admin::model::State& state, const std::string& name)
               {
                  auto found = common::algorithm::find_if( state.services, [&name]( auto& s){
                     return s.name == name;
                  });

                  if( found) 
                     return &( *found);

                  return nullptr;
               }
            } // service

            namespace instance
            {
               const manager::admin::model::instance::Sequential* find( const manager::admin::model::State& state, common::strong::process::id pid)
               {
                  auto found = common::algorithm::find_if( state.instances.sequential, [pid]( auto& i){
                     return i.process.pid == pid;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // instance

         } // <unnamed>
      } // local


      TEST( service_manager, startup_shutdown__expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            auto domain = local::domain();
         });

      }

      TEST( service_manager, configuration_default___expect_same)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto origin = local::configuration::load( local::configuration::base);

         auto model = casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get());

         EXPECT_TRUE( origin.service == model.service) << CASUAL_NAMED_VALUE( origin) << '\n' << CASUAL_NAMED_VALUE( model);
      }

      TEST( service_manager, configuration_routes___expect_same)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         routes: [ b, c, d]
         execution:
            timeout: 
               duration: 10ms
               contract: kill



)";

         auto domain = local::domain( configuration);

         auto origin = local::configuration::load( local::configuration::base, configuration);

         auto model = casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get());

         EXPECT_TRUE( origin.service == model.service) << CASUAL_NAMED_VALUE( origin.service) << '\n' << CASUAL_NAMED_VALUE( model.service);
      }

      TEST( service_manager, configuration_post)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   services:
      -  name: a
         routes: [ b, c, d]
         execution:
            timeout: 
               duration: 10ms
               contract: kill

      -  name: modify
         routes: [ x, y]
         execution:
            timeout: 
               duration: 10ms
               contract: kill

)");


         auto wanted = local::configuration::load( local::configuration::base, R"(
domain:
   services:
      -  name: b
         routes: [ x, y, z]
         execution:
            timeout: 
               duration: 20ms
               contract: linger

      -  name: modify
         routes: [ x, z]
         execution:
            timeout: 
               duration: 20ms
               contract: linger

)");

         // make sure the wanted differs (otherwise we're not testing anyting...)
         ASSERT_TRUE( wanted.service != casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get()).service);

         // post the wanted model (in transformed user representation)
         auto updated = casual::configuration::model::transform( 
            casual::domain::manager::unittest::configuration::post( casual::configuration::model::transform( wanted)));

         EXPECT_TRUE( wanted.service == updated.service) << CASUAL_NAMED_VALUE( wanted.service) << '\n' << CASUAL_NAMED_VALUE( updated.service);

      }


      namespace local
      {
         namespace
         {
            bool has_services( const std::vector< manager::admin::model::Service>& services, std::initializer_list< const char*> wanted)
            {
               return common::algorithm::all_of( wanted, [&services]( auto& s){
                  return common::algorithm::find_if( services, [&s]( auto& service){
                     return service.name == s;
                  });
               });
            }

         } // <unnamed>
      } // local

      

      TEST( service_manager, startup___expect_1_service_in_state)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto state = local::call::state();

         EXPECT_TRUE( local::has_services( state.services, { manager::admin::service::name::state()}));
         EXPECT_FALSE( local::has_services( state.services, { "non/existent/service"}));
      }


      TEST( service_manager, advertise_2_services_for_1_server__expect__3_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto state = local::call::state();

         ASSERT_TRUE( local::has_services( state.services, { manager::admin::service::name::state(), "service1", "service2"}));
         
         {
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).pid == common::process::id());
         }
         {
            auto service = local::service::find( state, "service2");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).pid == common::process::id());
         }
      }

      TEST( service_manager, advertise_A__route_B_A__expect_lookup_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         auto state = local::call::state();

         auto service = common::service::Lookup{ "B"}();
         // we expect the 'real-name' of the service to be replied
         EXPECT_TRUE( service.service.name == "A");


         EXPECT_TRUE( local::has_services( state.services, { "B"})) << CASUAL_NAMED_VALUE( state.services);
         EXPECT_TRUE( ! local::has_services( state.services, { "A"})) << CASUAL_NAMED_VALUE( state.services);
         
      }

      TEST( service_manager, service_a_execution_timeout_duration_1ms__advertise_a__lookup_a____expect__TPETIME__and_assassination_event)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         execution:
            timeout:
               duration: 1ms

)";

         auto domain = local::domain( configuration);

         // setup subscription to verify event
         common::event::subscribe( common::process::handle(), { common::message::event::process::Assassination::type()});

         service::unittest::advertise( { "a"});

         auto service = common::service::Lookup{ "a"}();
         ASSERT_TRUE( ! service.busy());

         common::message::service::call::Reply reply;
         EXPECT_TRUE( common::communication::device::blocking::receive( 
            common::communication::ipc::inbound::device(),
            reply));

         EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::timeout);

         common::message::event::process::Assassination event;
         EXPECT_TRUE( common::communication::device::blocking::receive( 
            common::communication::ipc::inbound::device(),
            event));

         EXPECT_TRUE( event.target == common::process::id());
         EXPECT_TRUE( event.contract == decltype( event.contract)::linger);
      }


      TEST( service_manager, env_variables__advertise_A__route_B_A__expect_lookup_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   default:
      environment:
         variables:
            - key: SA
              value: A
            - key: SB
              value: B
   services:
      - name: ${SA}
        routes: [ "${SB}"]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         auto state = local::call::state();


         EXPECT_TRUE( local::has_services( state.services, { "B"})) << CASUAL_NAMED_VALUE( state.services);
         EXPECT_TRUE( ! local::has_services( state.services, { "A"})) << CASUAL_NAMED_VALUE( state.services);
         
      }


      TEST( service_manager, advertise_A__route_B___expect_discovery_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);
         service::unittest::advertise( { "A"});

         // discover
         {            
            domain::message::discovery::Request request{ common::process::handle()};
            request.content.services = { "B"};
            domain::discovery::request( request);
            auto reply = common::message::reverse::type( request);
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            ASSERT_TRUE( reply.content.services.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.services.at( 0).name == "B") << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto state = local::call::state();
         EXPECT_TRUE( local::has_services( state.services, { "service1", "service2"})) << CASUAL_NAMED_VALUE( state.services);

         {
            auto instance = local::instance::find( state, common::process::id());
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == decltype( instance->state)::idle);
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown___expect_instance_removed_from_advertised_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});


         {
            auto state = local::call::state();
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.at( 0).pid == common::process::id());
         }

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( common::process::handle());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);
         }

         {
            auto state = local::call::state();
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.empty());
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown__lookup___expect_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( common::process::handle());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);
         }

         EXPECT_CODE({
            auto service = common::service::Lookup{ "service1"}();
         }, common::code::xatmi::no_entry);

         EXPECT_CODE({
            auto service = common::service::Lookup{ "service2"}();
         }, common::code::xatmi::no_entry);
      }

      TEST( service_manager, lookup_service__expect_service_to_be_busy)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         {
            auto service = common::service::Lookup{ "service2"}();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }
      }

      TEST( service_manager, unadvertise_service)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});
         service::unittest::unadvertise( { "service2"});

         // echo server has unadvertise this service. The service is
         // still "present" in service-manager with no instances. Hence it's absent
         EXPECT_CODE({
            auto service = common::service::Lookup{ "service2"}();
         }, common::code::xatmi::no_entry);
      }



      TEST( service_manager, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         EXPECT_CODE({
            auto service = common::service::Lookup{ "non-existent-service"}();
         }, common::code::xatmi::no_entry);
      }


      TEST( service_manager, service_lookup_service1__expect__busy_reply__send_ack____expect__idle_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         common::service::Lookup lookup{ "service2"};
         {
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }

         {
            // Send Ack
            {
               common::message::service::call::ACK message;
               message.metric.process = common::process::handle();

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  message);
            }

            // get next pending reply
            auto service = lookup();

            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }
      }


      TEST( service_manager, service_lookup_service1__service_lookup_service1_forward_context____expect_forward_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1"});

         auto forward = common::communication::instance::fetch::handle( forward::instance::identity.id);

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }


         {
            common::service::Lookup lookup{ "service1", common::service::Lookup::Context::forward};
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service1");

            // service-manager will let us think that the service is idle, and send us the process-handle to the forward-cache
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            EXPECT_TRUE( service.process);
            EXPECT_TRUE( service.process == forward);
         }
      }

      TEST( service_manager, service_lookup_service1__server_terminate__expect__service_error_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto correlation = []()
         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            
            return service.correlation;
         }();

         {
            common::message::event::process::Exit message;
            message.state.pid = common::process::id();
            message.state.reason = common::process::lifetime::Exit::Reason::core;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         {
            // we expect to get a service-call-error-reply
            common::message::service::call::Reply message;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               message);
            
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::service_error);
            EXPECT_TRUE( message.correlation == correlation);
         }
      }

      TEST( service_manager, advertise_a____prepare_shutdown___expect_reply_with_our_self)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a"});

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }

         {
            // we expect to get a prepare shutdown reply, with our self
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::blocking::receive( local::ipc::inbound(), reply);
            ASSERT_TRUE( reply.processes.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.processes.at( 0) == common::process::handle());
         }

         {
            // we expect service 'a' to be absent.
            auto lookup = common::service::Lookup{ "a"};
            EXPECT_CODE( lookup(), common::code::xatmi::no_entry);
         }
      }

      TEST( service_manager, advertise_a__lookup_a___prepare_shutdown___send_ack___expect_reply_with_our_self)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a"});

         // we reserve (lock) our instance to the 'call'
         auto lookup = common::service::Lookup{ "a"};

         // 'emulate' that a call is in progress (more like consuming the lookup reply...)
         EXPECT_TRUE( lookup().state == decltype( lookup().state)::idle);

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }

         // pretend a service call has been done
         service::unittest::send::ack( lookup());

         {
            // we expect to get a prepare shutdown reply, with our self
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::blocking::receive( local::ipc::inbound(), reply);
            ASSERT_TRUE( reply.processes.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.processes.at( 0) == common::process::handle());
         }

         {
            // we expect service 'a' to be absent.
            auto absent = common::service::Lookup{ "a"};
            EXPECT_CODE( absent(), common::code::xatmi::no_entry);
         }
      }

      TEST( service_manager, advertise_a_b_c_d___service_lookup_a_b_c_d___prepare_shutdown__expect_lookup_reply_1_idle__3_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a", "b", "c", "d"});

         auto idle = common::service::Lookup{ "a"};

         auto lookups = common::algorithm::container::emplace::initialize< std::vector< common::service::Lookup>>(  "b", "c", "d");

         EXPECT_TRUE( idle().state == decltype( idle().state)::idle);

         // the pending lookups should be busy (in the first round)
         for( auto& lookup : lookups)
            EXPECT_TRUE( lookup().state == decltype( lookup().state)::busy);

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }


         // We should get lookup reply with 'absent', hence, the pending lookups should throw 'no_entry'
         for( auto& lookup : lookups)
            EXPECT_CODE( lookup(), common::code::xatmi::no_entry);
         
         // we make service-manager think we've died
         {
            common::message::event::process::Exit message;
            message.state.pid = common::process::id();
            message.state.reason = common::process::lifetime::Exit::Reason::core;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         {
            // we expect to get a service-call-error-reply
            common::message::service::call::Reply message;
            
            common::communication::device::blocking::receive( local::ipc::inbound(), message);
            
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::service_error);
            EXPECT_TRUE( message.correlation == idle().correlation) << CASUAL_NAMED_VALUE( message) << "\n" << CASUAL_NAMED_VALUE( idle);
         }

         {
            // we expect to get a prepare shutdown reply, with no processes since we "died" mid call.
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::non::blocking::receive( local::ipc::inbound(), reply);
            EXPECT_TRUE( reply.processes.empty());
         }
      }

      TEST( service_manager, a_route_b__advertise_a__lookup_b_twice__expect_first_idle__send_ack___expect_second_idle)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: route

   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         // we reserve (lock) our instance to the 'call', via the route
         struct 
         {
            common::service::Lookup first{ "B"};
            common::service::Lookup second{ "B"};
         } lookup;

         // 'emulate' that a call is in progress (more like consuming the lookup reply...)
         EXPECT_TRUE( lookup.first().state == decltype( lookup.first().state)::idle);

         // expect a second "call" to get busy
         EXPECT_TRUE( lookup.second().state == decltype( lookup.second().state)::busy);

         // pretend the first call has been done
         service::unittest::send::ack( lookup.first());

         // expect the second to be idle now.
         EXPECT_TRUE( lookup.second().state == decltype( lookup.second().state)::idle);

      }

      TEST( service_manager, service_lookup__send_ack__expect_metric)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto start = platform::time::clock::type::now();
         auto end = start + std::chrono::milliseconds{ 2};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         // subscribe to metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});

         // Send Ack
         {
            common::message::service::call::ACK message;
            message.metric.process = common::process::handle();
            message.metric.service = "b";
            message.metric.parent = "a";
            message.metric.start = start;
            message.metric.end = end;
            message.metric.code = common::code::xatmi::service_fail;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         // wait for event
         {
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               event);
            
            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            EXPECT_TRUE( metric.service == "b");
            EXPECT_TRUE( metric.parent == "a");
            EXPECT_TRUE( metric.process == common::process::handle());
            EXPECT_TRUE( metric.start == start);
            EXPECT_TRUE( metric.end == end);
            EXPECT_TRUE( metric.code == common::code::xatmi::service_fail);
         }
      }

      TEST( service_manager, concurrent_advertise_same_instances_3_times_for_same_service___expect_one_instance_for_service)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         common::algorithm::for_n< 3>( []()
         {
            common::message::service::concurrent::Advertise message{ common::process::handle()};
            message.services.add.emplace_back( "concurrent_a", "", common::service::transaction::Type::none);
            
            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         });

         auto state = local::call::state();
         auto service = local::service::find( state, "concurrent_a");

         ASSERT_TRUE( service);
         auto& instances = service->instances.concurrent;
         ASSERT_TRUE( instances.size() == 1) << CASUAL_NAMED_VALUE( instances);
         EXPECT_TRUE( instances.at( 0).pid == common::process::id());
  
      }

   } // service
} // casual
