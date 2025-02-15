//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "queue/common/queue.h"
#include "queue/api/queue.h"
#include "queue/manager/admin/model.h"
#include "queue/manager/admin/services.h"
#include "queue/code.h"

#include "common/process.h"
#include "common/file.h"
#include "common/message/domain.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "common/code/signal.h"
#include "common/signal.h"
#include "common/communication/instance.h"
#include "common/serialize/macro.h"

#include "serviceframework/service/protocol/call.h"

#include "domain/manager/unittest/process.h"

#include <fstream>
#include <future>

namespace casual
{
   
   namespace queue
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto servers = R"(
domain: 
   name: queue-domain

   groups: 
      - name: base
      - name: queue
        dependencies: [ base]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-queue-manager
        memberships: [ queue]
)";

               constexpr auto queue = R"(
domain:
   queue:
      note: some note...
      default:
         queue: 
            retry:
               count: 3

      groups:
         - alias: A
           queuebase: ":memory:"
           queues:
            -  name: a1
               note: some note...
            -  name: a2
            -  name: a3
            -  name: delayed_100ms
               retry: { count: 10, delay: 100ms}
         - alias: B
           queuebase: ":memory:"
           queues:
            - name: b1
            - name: b2
            - name: b3

)";


            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return domain::manager::unittest::process( configuration::servers, std::forward< C>( configurations)...);
            }

            //! default domain
            auto domain()
            {
               return domain( configuration::queue);
            }

            namespace call
            {
               manager::admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state);

                  manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               std::vector< manager::admin::model::Message> messages( const std::string& queue)
               {
                  serviceframework::service::protocol::binary::Call call;
                  call << CASUAL_NAMED_VALUE( queue);
                  auto reply = call( manager::admin::service::name::messages::list);

                  std::vector< manager::admin::model::Message> result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }
            } // call

         } // <unnamed>

      } // local




      TEST( casual_queue, manager_startup)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         auto state = local::call::state();

         EXPECT_TRUE( state.groups.size() == 2);
         EXPECT_TRUE( state.queues.size() == ( 3 + 4) * 2)  << "state.queues.size(): " << state.queues.size();
      }


      TEST( casual_queue, manager_correct_retry_state)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto state = local::call::state();

         auto find_and_compare_queue = [&state]( const std::string& name, auto&& predicate)
         {
            auto found = common::algorithm::find( state.queues, name);
            if( ! found)
               return false;

            return predicate( *found);
         };

         auto retry_predicate = []( auto count, auto delay)
         {
            return [count, delay]( auto& queue)
            {
               return queue.retry.count == count && queue.retry.delay == delay;
            };
         };

         auto default_retry = retry_predicate( 3, std::chrono::seconds{ 0});

         EXPECT_TRUE( find_and_compare_queue( "a1", default_retry)) << CASUAL_NAMED_VALUE( state.queues);
         EXPECT_TRUE( find_and_compare_queue( "a2", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "a3", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "delayed_100ms", retry_predicate( 10, std::chrono::milliseconds{ 100})));

         EXPECT_TRUE( find_and_compare_queue( "b1", default_retry)) << CASUAL_NAMED_VALUE( state.queues);
         EXPECT_TRUE( find_and_compare_queue( "b2", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "b3", default_retry));
      }


      TEST( casual_queue, lookup_request_a1__expect_existence)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            // Send request
            ipc::message::lookup::Request request;
            request.process = common::process::handle();
            request.name = "a1";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);
         }

         {
            ipc::message::lookup::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.queue) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( casual_queue, lookup_request_non_existence__expect_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            // Send request
            ipc::message::lookup::Request request;
            request.process = common::process::handle();
            request.name = "non-existence";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);
         }

         {
            ipc::message::lookup::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_FALSE( reply.queue) << CASUAL_NAMED_VALUE( reply);
         }
      }


      TEST( casual_queue, enqueue_1_message___expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         const std::string payload{ "some message"};
         queue::Message message;
         message.payload.type = common::buffer::type::binary();
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "a1", message);
         auto messages = local::call::messages( "a1");

         EXPECT_TRUE( messages.size() == 1);
      }

      TEST( casual_queue, enqueue_1_message_with_no_payload___expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         queue::Message message;

         queue::enqueue( "a1", message);
         auto messages = local::call::messages( "a1");

         EXPECT_TRUE( messages.size() == 1);
      }


      TEST( casual_queue, enqueue_5_message___expect_5_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto count = 5;
         while( count-- > 0)
         {
            const std::string payload{ "some message"};
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::enqueue( "a1", message);
         }

         auto messages = local::call::messages( "a1");

         EXPECT_TRUE( messages.size() == 5);
      }
      namespace local
      {
         namespace
         {
            bool compare( const platform::time::point::type& lhs, const platform::time::point::type& rhs)
            {
               return std::chrono::time_point_cast< std::chrono::microseconds>( lhs)
                     == std::chrono::time_point_cast< std::chrono::microseconds>( rhs);
            }
         } // <unnamed>
      } // local

      TEST( casual_queue, enqueue_1_message__peek__information__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         const std::string payload{ "some message"};
         queue::Message message;
         {
            message.attributes.available = now;
            message.attributes.properties = "poop";
            message.attributes.reply = "a2";
            message.payload.type = common::buffer::type::binary();
            message.payload.data.assign( std::begin( payload), std::end( payload));
         }

         auto id = queue::enqueue( "a1", message);

         auto information = queue::peek::information( "a1");

         ASSERT_TRUE( information.size() == 1);
         auto& info = information.at( 0);
         EXPECT_TRUE( info.id == id);
         EXPECT_TRUE( local::compare( info.attributes.available, now));
         EXPECT_TRUE( info.attributes.properties == "poop") << "info: " << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.attributes.reply == "a2");
         EXPECT_TRUE( info.payload.type == common::buffer::type::binary());
         EXPECT_TRUE( info.payload.size == common::range::size( payload));

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, enqueue_1_message__peek_message__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         const std::string payload{ "some message"};

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.available = now;
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         auto id = enqueue();

         auto messages = queue::peek::messages( "a1", { id});

         ASSERT_TRUE( messages.size() == 1);
         auto& message = messages.at( 0);
         EXPECT_TRUE( message.id == id) << "message: " << CASUAL_NAMED_VALUE( message);
         EXPECT_TRUE( local::compare( message.attributes.available, now));
         EXPECT_TRUE( message.attributes.properties == "poop");
         EXPECT_TRUE( message.attributes.reply == "a2");
         EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, enqueue_1_message_with_no_payload__peek_message__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.available = now;
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
            }

            return queue::enqueue( "a1", message);
         };

         auto id = enqueue();

         auto messages = queue::peek::messages( "a1", { id});

         ASSERT_TRUE( messages.size() == 1);
         auto& message = messages.at( 0);
         EXPECT_TRUE( message.id == id) << "message: " << CASUAL_NAMED_VALUE( message);
         EXPECT_TRUE( local::compare( message.attributes.available, now));
         EXPECT_TRUE( message.attributes.properties == "poop");
         EXPECT_TRUE( message.attributes.reply == "a2");

         const decltype( message.payload.data) empty{};
         EXPECT_TRUE( message.payload.data == empty);

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( message.payload.data == empty);
         }
      }



      TEST( casual_queue, peek_remote_message___expect_throw)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // make sure casual-manager-queue knows about a "remote queue"
         {
            ipc::message::Advertise remote;

            remote.process.pid = common::strong::process::id{ 666};
            remote.process.ipc = common::strong::ipc::id{ common::uuid::make()};

            remote.queues.add.emplace_back( "remote-queue");

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), remote);
         }

         EXPECT_CODE({
            queue::peek::information( "remote-queue");
         },  queue::code::argument);
      }

      TEST( casual_queue, enqueue_1_message__dequeue_1_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const std::string payload{ "some message"};

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         enqueue();

         auto message = queue::dequeue( "a1");

         ASSERT_TRUE( message.size() == 1);
         EXPECT_TRUE( common::algorithm::equal( message.at( 0).payload.data, payload));
      }

      TEST( casual_queue, enqueue_1_message_delay_100ms__blocking_dequeue__expect_1_message_after_100ms)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const std::string payload{ "some message"};

         // message will be available at this absolute time
         auto available = platform::time::clock::type::now() + std::chrono::milliseconds{ 100};

         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.attributes.available = available;
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            queue::enqueue( "a1", message);
         }

         auto message = queue::blocking::dequeue( "a1");
         
         // we expect that at least 100ms has passed
         EXPECT_TRUE( platform::time::clock::type::now() > available);
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }

      TEST( casual_queue, enqueue_1_message___dequeue_rollback___expect_availiable_after_retry_delay_100ms)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const std::string payload{ "some message"};

         constexpr auto name = "delayed_100ms";

         // enqueue
         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }
            queue::enqueue( name, message);
         }

         auto start = platform::time::clock::type::now();

         // dequeue, and rollback
         {
            common::transaction::context().begin();
            ASSERT_TRUE( ! queue::dequeue( name).empty());
            common::transaction::context().rollback();
         }  

         // not available yet
         ASSERT_TRUE( queue::dequeue( name).empty());

         auto message = queue::blocking::dequeue( name);

         // we expect at least 100ms has passed, 
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 100});
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }

      namespace local
      {
         namespace
         {
            namespace blocking
            {
               template< typename IPC>
               void dequeue( IPC&& ipc, std::string name)
               {
                  queue::Lookup lookup{ std::move( name)};

                  ipc::message::group::dequeue::Request message;
                  message.name = lookup.name();
                  message.block = true;
                  message.process.pid = common::process::id();
                  message.process.ipc = ipc.connector().handle().ipc();

                  auto destination = lookup();
                  message.queue = destination.queue;

                  common::communication::device::blocking::send( destination.process.ipc, message);
               }
            } // blocking
         } // <unnamed>
      } // local

      TEST( casual_queue, blocking_dequeue_4_message__enqueue_4_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         common::communication::ipc::inbound::Device ipc1;
         common::communication::ipc::inbound::Device ipc2;
         common::communication::ipc::inbound::Device ipc3;
         common::communication::ipc::inbound::Device ipc4;

         local::blocking::dequeue( ipc1, "a1");
         local::blocking::dequeue( ipc2, "a1");
         local::blocking::dequeue( ipc3, "a1");
         local::blocking::dequeue( ipc4, "a1");

         const std::string payload{ "some message"};

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         enqueue();
         enqueue();
         enqueue();
         enqueue();

         auto dequeue = []( auto& ipc){
            ipc::message::group::dequeue::Reply message;
            common::communication::device::blocking::receive( ipc, message);
            return message;
         };

         {
            auto message = dequeue( ipc1);
            ASSERT_TRUE( message.message.size() == 1);
            EXPECT_TRUE( common::algorithm::equal( message.message.at( 0).payload, payload));
         }

         {
            auto message = dequeue( ipc2);
            ASSERT_TRUE( message.message.size() == 1);
            EXPECT_TRUE( common::algorithm::equal( message.message.at( 0).payload, payload));
         }

         {
            auto message = dequeue( ipc3);
            ASSERT_TRUE( message.message.size() == 1);
            EXPECT_TRUE( common::algorithm::equal( message.message.at( 0).payload, payload));
         }

         {
            auto message = dequeue( ipc4);
            ASSERT_TRUE( message.message.size() == 1);
            EXPECT_TRUE( common::algorithm::equal( message.message.at( 0).payload, payload));
         }
      }

      TEST( BACKWARD_casual_queue, queue_forward__enqueue_A3__expect_forward_to_B3)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: deprecated-forward

   groups: 
      - name: forward
        dependencies: [ queue]
   
   servers:
      - path: bin/casual-queue-forward-queue
        arguments: [ --forward, a3, b3]
        memberships: [ forward]
)";   

         auto domain = local::domain( local::configuration::queue, configuration);

         const std::string payload{ "some message"};

         // enqueue
         {
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::enqueue( "a3", message);
         }

         auto message = queue::blocking::dequeue( "b3");

         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }


      TEST( casual_queue, enqueue_1___remove_message____expect_0_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const std::string payload{ "some message"};
         queue::Message message;
         message.payload.type = common::buffer::type::binary();
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "a1", message);
         auto messages = local::call::messages( "a1");

         ASSERT_TRUE( messages.size() == 1);
         auto removed = queue::messages::remove( "a1", { messages.front().id});
         EXPECT_TRUE( messages.front().id == removed.at( 0));

         EXPECT_TRUE( local::call::messages( "a1").empty());
      }

      TEST( casual_queue, enqueue_5___clear_queue___expect_0_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const std::string payload{ "some message"};
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
            message.payload.data.assign( std::begin( payload), std::end( payload));

            common::algorithm::for_n< 5>( [&]()
            {
               queue::enqueue( "a1", message);
            });
         }

         auto messages = local::call::messages( "a1");
         EXPECT_TRUE( messages.size() == 5);

         auto cleared = queue::clear::queue( { "a1"});
         ASSERT_TRUE( cleared.size() == 1);
         EXPECT_TRUE( cleared.at( 0).queue == "a1");
         EXPECT_TRUE( cleared.at( 0).count == 5);

         EXPECT_TRUE( local::call::messages( "a1").empty());
      }

      TEST( casual_queue, configure_with_enviornment_variables)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: queue-environment

   environment:
      variables:
         -  key: Q_GROUP_PREFIX
            value: "group."
         -  key: Q_BASE
            value: ":memory:"
         -  key: Q_PREFIX
            value: "casual."
         -  key: Q_POSTFIX
            value: ".queue"

   queue:
      groups:
         -  alias: "${Q_GROUP_PREFIX}A"
            queuebase: "${Q_BASE}"
            queues:
               - name: ${Q_PREFIX}a1${Q_POSTFIX}
               - name: ${Q_PREFIX}a2${Q_POSTFIX}
               - name: ${Q_PREFIX}a3${Q_POSTFIX}

)";


         auto domain = local::domain( configuration);

         auto state = local::call::state();

         ASSERT_TRUE( state.groups.size() == 1) << CASUAL_NAMED_VALUE( state);
         auto& group = state.groups.at( 0);
         EXPECT_TRUE( group.name == "group.A") << CASUAL_NAMED_VALUE( group);
         EXPECT_TRUE( group.queuebase == ":memory:");
         
         auto order_queue = []( auto& l, auto& r){ return l.name < r.name;};
         common::algorithm::sort( state.queues, order_queue);
         ASSERT_TRUE( state.queues.size() == 6) << CASUAL_NAMED_VALUE( state.queues);

         EXPECT_TRUE( state.queues.at( 0).name == "casual.a1.queue");
         EXPECT_TRUE( state.queues.at( 1).name == "casual.a1.queue.error");
         EXPECT_TRUE( state.queues.at( 2).name == "casual.a2.queue");
         EXPECT_TRUE( state.queues.at( 3).name == "casual.a2.queue.error");
         EXPECT_TRUE( state.queues.at( 4).name == "casual.a3.queue");
         EXPECT_TRUE( state.queues.at( 5).name == "casual.a3.queue.error");

      }

      TEST( casual_queue, shutdown_during_non_blocking_queue_lookup__expect_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         queue::Message message;
         {
            message.payload.type = common::buffer::type::binary();
            message.payload.data = common::unittest::random::binary( 43);
         }

         auto id = queue::enqueue( "a1", message);
         EXPECT_TRUE( id) << "id: " << id;

         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }

      }

      TEST( casual_queue, shutdown_during_blocking_dequeue__expect_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         EXPECT_CODE( 
         {  
            queue::blocking::dequeue( "a1");
         }, queue::code::no_message);

         // expect shutdown to 'stay'
         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }
      }

      TEST( casual_queue, enqueue_message__shutdown_during_blocking_dequeue___expect__message_and_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            queue::Message message;
            {
               message.payload.type = common::buffer::type::binary();
               message.payload.data = common::unittest::random::binary( 43);
            }
            queue::enqueue( "a1", message);
         }

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         EXPECT_NO_THROW(
         {  
            queue::blocking::dequeue( "a1");
         });

         // expect shutdown to 'stay'
         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }
      }

      namespace local
      {
         namespace
         {
            namespace zombie
            {
               constexpr auto configuration_with_queue_B = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ".casual/zombies.db"

            queues:
               - name: queue_A
               - name: queue_B
)";

               constexpr auto configuration_without_queue_B = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ".casual/zombies.db"
            queues:
               - name: queue_A
)";

               constexpr auto database{".casual/zombies.db"};
            } // zombie
         } // <unnamed>
      } // local

      TEST( casual_queue, detect_zombie_queue__remove_empty_queues__no_zombies)
      {
         common::unittest::Trace trace;
         common::file::scoped::Path database{ local::zombie::database};

         // sink child signals 
         common::signal::callback::registration< common::code::signal::child>( [](){});

         {
            auto domain = local::domain( local::zombie::configuration_with_queue_B);
            auto state = local::call::state();

            EXPECT_TRUE( state.queues.size() == 2 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();
         }

         {
            auto domain = local::domain( local::zombie::configuration_without_queue_B);
            auto state = local::call::state();

            EXPECT_TRUE( state.queues.size() == 1 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();
         }
      }

      TEST( casual_queue, detect_zombie_queue__add_message__cannot_remove_queue__1_zombie_queue)
      {
         common::unittest::Trace trace;
         common::file::scoped::Path database{ local::zombie::database};

         // sink child signals 
         common::signal::callback::registration< common::code::signal::child>( [](){});

         {
            auto domain = local::domain( local::zombie::configuration_with_queue_B);
            auto state = local::call::state();

            EXPECT_TRUE( state.queues.size() == 2 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();

            queue::Message message;

            queue::enqueue( "queue_B", message);
            auto messages = local::call::messages( "queue_B");

            EXPECT_TRUE( messages.size() == 1);
         }

         {
            auto domain = local::domain( local::zombie::configuration_without_queue_B);
            auto state = local::call::state();

            EXPECT_TRUE( state.queues.size() == 1 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 1 * 2) << "state.zombies.size(): " << state.zombies.size();
 
            {
               // Send request
               ipc::message::lookup::Request request;
               request.process = common::process::handle();
               // queue_B should not be advertised now because it is a zombie
               request.name = "queue_B";

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::queue::manager::device(), 
                  request);

               ipc::message::lookup::Reply reply;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

               EXPECT_FALSE( reply.queue) << CASUAL_NAMED_VALUE( reply);
            }
         }
      }
   } // queue
} // casual
