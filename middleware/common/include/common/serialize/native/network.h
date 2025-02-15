//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/native/binary.h"
#include "common/network/byteorder.h"
#include "common/traits.h"

#include "common/cast.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace native
         {
            namespace binary
            {
               namespace network
               {
                  using size_type = platform::size::type;
                  
                  namespace detail
                  {

                     template< typename T>
                     inline constexpr auto is_network_array_v = common::traits::is::binary::like_v< T>;

                     template< typename T>
                     constexpr auto cast( T value) -> std::enable_if_t< ! std::is_enum< T>::value, T>
                     {
                        return value;
                     }

                     template< typename T>
                     constexpr auto cast( T value) -> std::enable_if_t< std::is_enum< T>::value, decltype( cast::underlying( value))>
                     {
                        return common::cast::underlying( value);
                     }

                  } // detail

                  struct Policy
                  {
                     template< typename T>
                     static auto write( const T& value, platform::binary::type& buffer) 
                        -> std::enable_if_t< ! detail::is_network_array_v< T>>
                     {
                        auto net_value = common::network::byteorder::encode( detail::cast( value));
                        memory::append( net_value, buffer);
                     }

                     template< typename T>
                     static auto write( const T& value, platform::binary::type& buffer)
                        -> std::enable_if_t< detail::is_network_array_v< T>>
                     {
                        memory::append( value, buffer);
                     }

                     template<typename T>
                     static void write_size( const T& size, platform::binary::type& buffer)
                     {
                        const auto encoded = common::network::byteorder::size::encode( size);
                        memory::append( encoded, buffer);
                     }


                     template< typename T>
                     static std::enable_if_t< ! detail::is_network_array_v< T>, size_type>
                     read( const platform::binary::type& buffer, size_type offset, T& value)
                     {
                        using value_type = decltype( detail::cast( value));
                        common::network::byteorder::type< value_type> net_value;
                        offset = memory::copy( buffer, offset, net_value);
                        value = static_cast< T>( common::network::byteorder::decode< value_type>( net_value));

                        return offset;
                     }

                     template< typename T>
                     static std::enable_if_t< detail::is_network_array_v< T>, size_type>
                     read( const platform::binary::type& buffer, size_type offset, T& value)
                     {
                        return memory::copy( buffer, offset, value);
                     }

                     template<typename T>
                     static size_type read_size( const platform::binary::type& buffer, size_type offset, T& value)
                     {
                        decltype(common::network::byteorder::size::encode( value)) encoded;
                        offset = memory::copy( buffer, offset, encoded);
                        value = common::network::byteorder::size::decode<T>( encoded);
                        return offset;
                     }


                  };

                  using Reader = basic_reader< Policy>;

                  using Writer = basic_writer< Policy>;

                  namespace create
                  {
                     struct Writer
                     {
                        inline auto operator () () const
                        {
                           return network::Writer{};
                        }
                     };

                     struct Reader
                     {
                        inline auto operator () ( platform::binary::type& buffer) const
                        {
                           return network::Reader{ buffer};
                        }
                     };

                  } // create
               } // network
            } // binary

            namespace create
            {
              
               template<>
               struct reverse< binary::network::create::Writer> { using type = binary::network::create::Reader;};

               template<>
               struct reverse< binary::network::create::Reader> { using type = binary::network::create::Writer;};

            } // create

         } // native
         namespace traits
         {
            namespace is
            {
               namespace network
               {
                  template<>
                  struct normalizing< serialize::native::binary::network::Reader>: std::true_type {};

                  template<>
                  struct normalizing< serialize::native::binary::network::Writer>: std::true_type {};
               } // network
            } // is 
         } // traits
      } // serialize
   } // common
} // casual


