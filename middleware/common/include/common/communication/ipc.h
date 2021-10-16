//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc/message.h"
#include "common/communication/device.h"
#include "common/communication/log.h"

#include "casual/platform.h"
#include "common/strong/id.h"
#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/flag.h"


#include <sys/un.h>

namespace casual
{
   namespace common::communication::ipc
   {
      namespace message
      {
         namespace transport
         {
            struct Header
            {
               //! Which logical type of message this transport is carrying
               //! @attention has to be the first bytes in the message
               common::message::Type type;

               using correlation_type = Uuid::uuid_type;

               //! The message correlation id
               correlation_type correlation;

               //! which offset this transport message represent of the complete message
               std::int64_t offset;

               //! Size of payload in this transport message
               std::int64_t count;

               //! size of the logical complete message
               std::int64_t size;

               friend std::ostream& operator << ( std::ostream& out, const Header& value);
            };

            namespace header
            {
               constexpr std::int64_t size() { return sizeof( Header);}   
            } // header

            namespace max::size
            {
               constexpr std::int64_t message() { return platform::ipc::transport::size;}
               constexpr std::int64_t payload() { return size::message() - header::size();}               
            } // max::size

            



         } // transport

         struct Transport
         {
            using correlation_type = Uuid::uuid_type;

            using payload_type = std::array< char, transport::max::size::payload()>;
            using range_type = range::type_t< payload_type>;
            using const_range_type = range::const_type_t< payload_type>;

            struct message_t
            {
               transport::Header header;
               payload_type payload;

            } message{}; // note the {} which initialize the memory to 0:s


            //static_assert( transport::max_message_size() - transport::max::size::payload() < transport::max::size::payload(), "Payload is to small");
            //static_assert( std::is_trivially_copyable< message_t>::value, "Message has to be trivially copyable");
            //static_assert( ( transport::header_size() + transport::max::size::payload()) == transport::max_message_size(), "something is wrong with padding");


            inline Transport() = default;

            inline Transport( common::message::Type type, platform::size::type size)
            {
               message.header.type = type;
               message.header.size = size;
            }

            //! @return the message type
            inline common::message::Type type() const { return static_cast< common::message::Type>( message.header.type);}

            inline const_range_type payload() const
            {
               return range::make( std::begin( message.payload), message.header.count);
            }

            inline const auto& correlation() const noexcept { return message.header.correlation;}
            inline auto& correlation() noexcept { return message.header.correlation;}

            //! @return payload size
            inline platform::size::type payload_size() const noexcept { return message.header.count;}

            //! @return the offset of the logical complete message this transport
            //!    message represent.
            inline platform::size::type payload_offset() const noexcept { return message.header.offset;}

            //! @return the size of the complete logical message
            inline platform::size::type complete_size() const noexcept { return message.header.size;}

            //! @return the total size of the transport message including header.
            inline platform::size::type size() const noexcept { return transport::header::size() + payload_size();}

            inline auto data() noexcept { return reinterpret_cast< char*>( &message);}
            inline auto data() const noexcept { return reinterpret_cast< const char*>( &message);}

            inline auto header_data() noexcept { return reinterpret_cast< char*>( &message.header);}
            inline auto payload_data() noexcept { return reinterpret_cast< char*>( &message.payload);}


            auto begin() { return std::begin( message.payload);}
            inline auto begin() const { return std::begin( message.payload);}
            auto end() { return begin() + message.header.count;}
            auto end() const { return begin() + message.header.count;}

            template< typename R>
            void assign( R&& range)
            {
               message.header.count = range.size();
               assert( message.header.count <= transport::max::size::payload());

               algorithm::copy( range, std::begin( message.payload));
            }

            friend std::ostream& operator << ( std::ostream& out, const Transport& value);
         };

         inline platform::size::type offset( const Transport& value) { return value.message.header.offset;}

      } // message

      namespace detail
      {
         struct Descriptor
         {
            Descriptor() = default;
            inline Descriptor( strong::file::descriptor::id descriptor)
               : m_descriptor{ descriptor} {}
            Descriptor( Descriptor&& other) noexcept;
            Descriptor& operator = ( Descriptor&& other) noexcept;
            ~Descriptor();

            inline explicit operator bool () const { return predicate::boolean( m_descriptor);}

            inline auto id() const noexcept { return m_descriptor;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_descriptor, "value");
            );

         private:
            strong::file::descriptor::id m_descriptor;
         };         
      } // detail




      struct Handle
      {
         Handle() = default;
         inline Handle( strong::file::descriptor::id descriptor, strong::ipc::id ipc)
            : m_descriptor( descriptor), m_ipc( std::move( ipc))
         {}

         inline auto descriptor() const { return m_descriptor.id();}
         inline strong::ipc::id ipc() const { return m_ipc;}

         inline explicit operator bool () const { return predicate::boolean( m_descriptor);}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_descriptor, "descriptor");
            CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
         )

      private:
         detail::Descriptor m_descriptor;
         strong::ipc::id m_ipc;
      };

      static_assert( sizeof( Handle) == sizeof( strong::file::descriptor::id) + sizeof( strong::ipc::id), "padding problem");



   

      namespace policy
      {
         using cache_type = std::vector< message::Complete>;
         using cache_range_type = range::type_t< cache_type>;

         namespace blocking
         {
            cache_range_type receive( const Handle& handle, cache_type& cache);
            strong::correlation::id send( const Handle& destination, const message::Complete& complete);
         } // blocking


         struct Blocking
         {

            template< typename Connector>
            cache_range_type receive( Connector&& connector, cache_type& cache)
            {
               return policy::blocking::receive( connector.handle(), cache);
            }

            template< typename Connector>
            auto send( Connector&& connector, const message::Complete& complete)
            {
               return policy::blocking::send( connector.handle(), complete);
            }
         };

         namespace non
         {
            namespace blocking
            {
               cache_range_type receive( const Handle& handle, cache_type& cache);
               strong::correlation::id send( const Handle& destination, const message::Complete& complete);
            } // blocking

            struct Blocking
            {
               template< typename Connector>
               cache_range_type receive( Connector&& connector, cache_type& cache)
               {
                  return policy::non::blocking::receive( connector.handle(), cache);
               }

               template< typename Connector>
               auto send( Connector&& connector, const message::Complete& complete)
               {
                  return policy::non::blocking::send( connector.handle(), complete);
               }
            };

         } // non

      } // policy


      namespace inbound
      {
         struct Connector
         {
            using transport_type = message::Transport;
            using blocking_policy = policy::Blocking;
            using non_blocking_policy = policy::non::Blocking;
            using cache_type = policy::cache_type;

            Connector();
            ~Connector();

            inline const Handle& handle() const { return m_handle;}
            inline auto descriptor() const { return m_handle.descriptor();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_handle, "handle");
            )

         private:
            Handle m_handle;
            detail::Descriptor m_writer;
         };

         using Device = device::Inbound< Connector>;


         Device& device();
         inline const Handle& handle() { return device().connector().handle();}
         inline strong::ipc::id ipc() { return device().connector().handle().ipc();}

      } // inbound

      namespace outbound
      {

         struct Connector
         {
            using complete_type = message::Complete;
            using transport_type = message::Transport;
            using blocking_policy = policy::Blocking;
            using non_blocking_policy = policy::non::Blocking;

            Connector() = default;
            Connector( strong::ipc::id destination);
            

            inline const Handle& handle() const { return m_handle;}
            inline auto descriptor() const { return m_handle.descriptor();}

            inline void reconnect() const { throw; }

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_handle, "handle");
            )

         protected:
            Handle m_handle;
            detail::Descriptor m_writer;
         };

         template< typename S>
         using basic_device = device::Outbound< Connector, S>;

         using Device = device::Outbound< Connector>;

      } // outbound


      template< typename D, typename M, typename Device = inbound::Device>
      auto call(
            D&& destination,
            M&& message,
            Device& device = inbound::device())
      {
         return device::call( std::forward< D>( destination), std::forward< M>( message), device);
      }

      //! @returns the received message of type `R`
      template< typename R, typename... Ts>
      R receive( Ts&&... ts)
      {
         R reply;
         device::blocking::receive( inbound::device(), reply, std::forward< Ts>( ts)...);
         return reply;
      }


      bool remove( strong::ipc::id id);
      bool remove( const process::Handle& owner);

      bool exists( strong::ipc::id id);

   } // common::communication::ipc

   namespace common::communication::device
   {
      template<>
      struct customization_point< strong::ipc::id>
      {
         using non_blocking_policy = ipc::policy::non::Blocking;
         using blocking_policy = ipc::policy::Blocking;

         template< typename... Ts>
         static auto send( strong::ipc::id ipc, Ts&&... ts) 
         {
            return device::send( ipc::outbound::Device{ ipc}, std::forward< Ts>( ts)...);
         }
      };
   } // common::communication::device

   
} // casual