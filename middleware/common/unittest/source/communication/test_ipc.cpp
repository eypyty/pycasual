//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/communication/ipc.h"
#include "common/message/domain.h"
#include "common/message/service.h"
#include "common/unittest/eventually/send.h"

#include <random>
#include <thread>

namespace casual
{
   namespace common::communication::ipc
   {
      namespace local
      {
         namespace
         {
            namespace payload
            {
               const platform::binary::type& get()
               {
                  auto create = [](){
                     platform::binary::type result( 1000);
                     algorithm::numeric::iota( result, 0);
                     std::random_device rd;
                     std::shuffle( std::begin( result), std::end( result), std::mt19937{ rd()});

                     return result;
                  };
                  static auto result = create();
                  return result;
               }

               using range_type = range::type_t< platform::binary::type>;

               std::vector< ipc::message::Transport> parts( std::size_t size, common::message::Type type, const platform::binary::type& payload = get())
               {
                  std::vector< ipc::message::Transport> result;

                  auto current = std::begin( payload);
                  while( current != std::end( payload))
                  {
                     auto last = current + size;
                     if( last > std::end( payload))
                     {
                        last = std::end( payload);
                     }
                     ipc::message::Transport transport;
                     transport.message.header.type = type;
                     transport.message.header.size = payload.size();
                     transport.message.header.count = std::distance( current, last);
                     transport.message.header.offset = std::distance( std::begin( payload), current);

                     algorithm::copy( range::make( current, last), std::begin( transport.message.payload));

                     result.push_back( transport);
                     current = last;
                  }

                  return result;
               }

               message::Complete complete( const std::vector< ipc::message::Transport>& parts)
               {
                  assert( ! parts.empty());

                  message::Complete message{ parts.front()};

                  for( auto& transport : range::make( std::begin( parts) + 1, std::end( parts)))
                  {
                     message.add( transport);
                  }
                  return message;
               }


            } // payload

         } // <unnamed>
      } // local

      TEST( casual_common_communication_message, complete_add__ordered)
      {
         common::unittest::Trace trace;

         auto complete = local::payload::complete( local::payload::parts( 100, common::message::Type::event_service_call));

         EXPECT_TRUE( static_cast< bool>( complete));
         EXPECT_TRUE( complete.complete()) << "complete.unhandled().size()" << complete.unhandled().size();
         if( ! complete.complete())
         {
            for( auto a : complete.unhandled())
            {
               std::cerr << "size: " << a.size() << '\n';
            }
         }

         EXPECT_TRUE( complete.payload == local::payload::get());
      }

      TEST( casual_common_communication_message, complete_add__reverse_ordered)
      {
         common::unittest::Trace trace;

         auto parts = local::payload::parts( 100, common::message::Type::event_service_call);
         algorithm::reverse( parts);
         auto complete = local::payload::complete( parts);

         EXPECT_TRUE( static_cast< bool>( complete));
         EXPECT_TRUE( complete.complete()) << "complete.unhandled().size()" << complete.unhandled().size();
         if( ! complete.complete())
         {
            for( auto a : complete.unhandled())
            {
               std::cerr << "size: " << a.size() << '\n';
            }
         }
         EXPECT_TRUE( complete.payload == local::payload::get());
      }


      TEST( common_communication_ipc, instantiate)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device device;
      }


      TEST( common_communication_ipc, non_blocking_receive__expect_no_messages)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device device;

         common::message::domain::process::lookup::Reply message;
         EXPECT_FALSE( ( device.receive( message, ipc::policy::non::Blocking{})));

      }

      TEST( common_communication_ipc, send_receive__small_message)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;


         auto send = []( strong::ipc::id id)
         {
            common::message::domain::process::lookup::Reply message;
            return device::non::blocking::send( id, message);
         };

         auto correlation = send( destination.connector().handle().ipc());

         common::message::domain::process::lookup::Reply message;
         EXPECT_TRUE( ( device::non::blocking::receive( destination, message, correlation)));
      }

      TEST( common_communication_ipc, send_receive__large_message___flush__expect_cached)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;

         const auto binary = unittest::random::binary( 12 * 1024);

         auto correlation = unittest::eventually::send( destination.connector().handle().ipc(), unittest::Message{ binary});
         
         while( destination.complete() == 0)
            destination.flush();

         unittest::Message message;
         EXPECT_TRUE( device::blocking::receive( destination, message, correlation));
         EXPECT_TRUE( message.payload == binary);

         EXPECT_TRUE( destination.complete() == 0);
         EXPECT_TRUE( destination.size() == 0);
      }

      TEST( common_communication_ipc, send_receive__10_large_message___flush__expect_cached)
      {
         common::unittest::Trace trace;

         ipc::inbound::Device destination;

         constexpr platform::size::type count = 10;

         const auto binary = unittest::random::binary( 12 * 1024);

         auto correlations = algorithm::generate_n< count>( [ipc = destination.connector().handle().ipc(), &binary]()
         {
            return unittest::eventually::send( ipc, unittest::Message{ binary});
         });

         while( destination.complete() < count)
            destination.flush();

         for( auto& correlation : correlations)
         {
            unittest::Message message;
            EXPECT_TRUE( device::blocking::receive( destination, message, correlation));
            EXPECT_TRUE( message.payload == binary);
         };

         EXPECT_TRUE( destination.complete() == 0);
         EXPECT_TRUE( destination.size() == 0);
      }


      TEST( common_communication_ipc, send_receive__1_exactly_transport_size__expect_exactly_1_transport_message)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( ipc::message::transport::max_payload_size());

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         {

            ipc::message::Transport transport;
            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == 0);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

            // we expect no more transports
            {
               ipc::message::Transport dummy;
               EXPECT_FALSE( ipc::native::receive( ipc::inbound::handle(), dummy, ipc::native::Flag::non_blocking));
            }

            message::Complete complete{ transport};
            serialize::native::complete( complete, receive_message);
         }

         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }


      TEST( common_communication_ipc, send_receive__1__2x_transport_size__expect_exactly_2_transport_message)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 2 * ipc::message::transport::max_payload_size());
         unittest::eventually::send( ipc::inbound::ipc(), send_message);


         unittest::Message receive_message;
         {

            ipc::message::Transport transport;
            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == 0);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

            message::Complete complete{ transport};

            EXPECT_TRUE( ipc::native::receive( ipc::inbound::handle(), transport, ipc::native::Flag::none));
            EXPECT_TRUE( transport.message.header.offset == transport.message.header.count);
            EXPECT_TRUE( transport.message.header.count == ipc::message::transport::max_payload_size());

            // we expect no more transports
            {
               ipc::message::Transport dummy;
               EXPECT_FALSE( ipc::native::receive( ipc::inbound::handle(), dummy, ipc::native::Flag::non_blocking));
            }
            complete.add( transport);

            serialize::native::complete( complete, receive_message);
         }

         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }


      TEST( common_communication_ipc, send_receive__1__10x_transport_size__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 10 * ipc::message::transport::max_payload_size());

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message);
         EXPECT_TRUE( ( algorithm::equal( receive_message.payload, send_message.payload)));
      }

      TEST( common_communication_ipc, send_receive__1__message_size_1103__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         auto send_message = unittest::message::transport::size( 1103);
         send_message.correlation = strong::correlation::id::emplace( uuid::make());

         unittest::eventually::send( ipc::inbound::ipc(), send_message);

         unittest::Message receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message);
         EXPECT_TRUE( send_message == receive_message);

      }



      TEST( common_communication_ipc, send_receive__10__3x_transport_size__expect_correct_assembly)
      {
         common::unittest::Trace trace;

         std::vector< strong::correlation::id> correlations;

         auto send_message = unittest::message::transport::size( 3 * ipc::message::transport::max_payload_size());

         for( int count = 10; count > 0; --count)
         {
            correlations.push_back( unittest::eventually::send( ipc::inbound::ipc(), send_message));
         }

         // We reverse the order we fetch the messages
         for( auto& correlation : common::algorithm::reverse( correlations))
         {
            unittest::Message receive_message;

            device::blocking::receive( ipc::inbound::device(), receive_message, correlation);

            ASSERT_TRUE( receive_message.correlation == correlation);
            ASSERT_TRUE( algorithm::equal( receive_message.payload, send_message.payload));
         }
      }

      TEST( common_communication_ipc, send_receive__service_call_reply)
      {
         common::unittest::Trace trace;

         common::message::service::call::Reply message;
         {
            message.correlation = strong::correlation::id::emplace( uuid::make());
            message.execution = strong::execution::id::emplace( uuid::make());
            message.transaction.trid = transaction::id::create( process::handle());
            message.transaction.state = common::message::service::Transaction::State::rollback;
            message.buffer.type = ".binary";
            message.code.result = common::code::xatmi::ok;
            message.code.user = 0;
            message.buffer.memory = unittest::random::binary( 1200);
         }

         const auto correlation = unittest::eventually::send( ipc::inbound::ipc(), message);

         common::message::service::call::Reply receive_message;
         device::blocking::receive( ipc::inbound::device(), receive_message, correlation);


         EXPECT_TRUE( receive_message.correlation == correlation);
         EXPECT_TRUE( receive_message.execution == message.execution);
         EXPECT_TRUE( receive_message.transaction.trid == message.transaction.trid);
         EXPECT_TRUE( receive_message.transaction.state == message.transaction.state);
         EXPECT_TRUE( receive_message.buffer.memory == message.buffer.memory);
      }

      namespace local
      {
         namespace
         {
            struct Message : unittest::Message
            {
               using unittest::Message::Message;
               
               platform::size::type index = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  unittest::Message::serialize( archive);
                  CASUAL_SERIALIZE( index);
               })
            };

            void send_messages( strong::ipc::id destination, Message message, platform::size::type count)
            {
               for( ; message.index < count; ++message.index)
               {
                  device::blocking::send( destination, message);
               }
            };


            void test_send( platform::size::type size, platform::size::type count)
            {
               local::Message origin( size);
               
               auto sender = std::thread{ &local::send_messages, ipc::inbound::ipc(), origin, count};

               // TODO: May probably be replaced by C++20 std::jthread
               const auto joiner = execute::scope( [&sender]() { sender.join();});

               local::Message message;
               for( auto index = 0; index < count; ++index)
               {
                  device::blocking::receive( ipc::inbound::device(), message);
                  ASSERT_EQ( message.index, index);
                  ASSERT_EQ( message.payload, origin.payload);
               }

            }
         } // <unnamed>
      } // local

      TEST( common_communication_ipc, send_receive_100_messages__1kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024, 100);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__1kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__10kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 10, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_1000_messages__100kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 100, 1000);
      }

      TEST( DISABLED_common_communication_ipc, send_receive_10000_messages__100kB)
      {
         common::unittest::Trace trace;

         local::test_send( 1024 * 100, 10000);
      }

   } // common::communication::ipc
} // casual
