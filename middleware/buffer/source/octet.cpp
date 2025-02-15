//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/octet.h"

#include "common/buffer/pool.h"
#include "common/memory.h"
#include "casual/platform.h"
#include "common/exception/handle.h"

namespace casual
{
   namespace buffer
   {
      namespace octet
      {
         namespace
         {

            using size_type = platform::size::type;
            using const_data_type = platform::binary::type::const_pointer;
            using data_type = platform::binary::type::pointer;


            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               //! Implement Buffer::transport
               size_type transport( const size_type user_size) const
               {

                  // Just ignore user-size all together
                  return payload.memory.size();
               }

            };


            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  static const types_type result
                  {
                     common::buffer::type::combine( CASUAL_OCTET),
                     common::buffer::type::combine( CASUAL_OCTET, CASUAL_OCTET_XML),
                     common::buffer::type::combine( CASUAL_OCTET, CASUAL_OCTET_JSON),
                     common::buffer::type::combine( CASUAL_OCTET, CASUAL_OCTET_YAML),
                  };

                  return result;
               }

               platform::buffer::raw::type allocate( const std::string& type, const platform::binary::size::type size)
               {
                  auto& buffer = m_pool.emplace_back( type, size);

                  // GCC returns null for std::vector::data with capacity zero
                  if( ! size) 
                     buffer.payload.memory.reserve( 1);

                  return buffer.payload.memory.data();
               }


               platform::buffer::raw::type reallocate( const platform::buffer::raw::immutable::type handle, const platform::binary::size::type size)
               {
                  const auto result = find( handle);

                  m_pool.back().payload.memory.resize( size);

                  if( size)
                  {
                     // Allow user to reduce allocation
                     result->payload.memory.shrink_to_fit();
                  }
                  else
                  {
                     // GCC returns null for std::vector::data with capacity zero
                     m_pool.back().payload.memory.reserve( 1);
                  }

                  return result->payload.memory.data();
               }
            };

         } // <unnamed>

      } // octet


   } // buffer

   template class common::buffer::pool::Registration< buffer::octet::Allocator>;


   namespace buffer
   {
      namespace octet
      {
         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            int error() noexcept
            {
               try
               {
                  throw;
               }
               catch( const std::bad_alloc&)
               {
                  return CASUAL_OCTET_OUT_OF_MEMORY;
               }
               catch( ...)
               {
                  if( common::exception::capture().code() == common::code::xatmi::argument)
                     return CASUAL_OCTET_INVALID_HANDLE;

                  return CASUAL_OCTET_INTERNAL_FAILURE;
               }
            }


            int explore( const char* const handle, const char** name, size_type* const size) noexcept
            {
               //const trace trace( "octet::explore");

               try
               {
                  const auto& buffer = pool_type::pool.get( handle);


                  if( name)
                  {
                     *name = std::get< 1>( common::algorithm::split( buffer.payload.type, '/')).data();
                  }
                  if( size) *size = buffer.payload.memory.size();
               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_OCTET_SUCCESS;
            }


            int set( char** const handle, const_data_type data, const size_type size) noexcept
            {
               //const trace trace( "string::set");

               try
               {
                  auto& buffer = pool_type::pool.get( *handle);

                  *handle = casual::common::algorithm::copy(
                     casual::common::range::make( data, size),
                     buffer.payload.memory).data();

               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_OCTET_SUCCESS;

            }

            int get( const char* const handle, const_data_type& data, size_type& size) noexcept
            {
               //const trace trace( "octet::get");

               try
               {
                  const auto& buffer = pool_type::pool.get( handle);

                  data = buffer.payload.memory.data();
                  size = buffer.payload.memory.size();

               }
               catch( ...)
               {
                  return error();
               }

               return CASUAL_OCTET_SUCCESS;

            }

         } // <unnamed>

      } // octet

   } // buffer


} // casual


const char* casual_octet_description( const int code)
{
   switch( code)
   {
      case CASUAL_OCTET_SUCCESS:
         return "Success";
      case CASUAL_OCTET_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_OCTET_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_OCTET_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_OCTET_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int casual_octet_explore_buffer( const char* const handle, const char** name, long* const size)
{
   return casual::buffer::octet::explore( handle, name, size);
}


int casual_octet_set( char** handle, const char* const data, const long size)
{
   if( size < 0)
   {
      return CASUAL_OCTET_INVALID_ARGUMENT;
   }

   return casual::buffer::octet::set( handle, data, size);

}

int casual_octet_get( const char* const handle, const char** data, long* const size)
{
   return casual::buffer::octet::get( handle, *data, *size);
}
