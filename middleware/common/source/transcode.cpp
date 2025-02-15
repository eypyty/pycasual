//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transcode.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/exception/handle.h"

#include "common/result.h"

#include <cppcodec/base64_rfc4648.hpp>

#include <locale>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <sstream>
// codecvt is not part of GCC yet ...
//#include <codecvt>
// ... so we have to use the cumbersome iconv instead
#include <iconv.h>
#include <clocale>
#include <cerrno>
#include <cstdlib>
#include <cassert>


namespace casual
{
   namespace common
   {
      namespace transcode
      {

         namespace base64
         {
            namespace detail
            {
               platform::size::type encode( const Data source, Data target)
               {
                  // calls abort() if target size is insufficient
                  return cppcodec::base64_rfc4648::encode(
                     static_cast<char*>(target.memory),
                     target.bytes,
                     static_cast<const char*>(source.memory),
                     source.bytes); 
               }

               platform::size::type decode( const char* first, const char* last, char* dest_first, char* dest_last)
               {
                  try
                  {
                     // calls abort() if target size is insufficient
                     return cppcodec::base64_rfc4648::decode(
                        dest_first,
                        std::distance( dest_first, dest_last),
                        first,
                        std::distance( first, last));
                  }
                  catch( const std::exception& e)
                  {
                     code::raise::error( code::casual::failed_transcoding, "base64 decode failed");
                  }
               }

            } // detail


            platform::binary::type decode( const std::string& value)
            {
               std::vector<char> result( (value.size() / 4) * 3);

               result.resize( detail::decode( value.data(), value.data() + value.size(), result.data(), result.data() + result.size()));

               return result;
            }

         } // base64

         namespace local
         {
            namespace
            {
               class converter
               {
               public:
                  converter( std::string_view source, std::string_view target)
                     : m_descriptor( iconv_open( target.data(), source.data()))
                  {
                     if( m_descriptor == reinterpret_cast< iconv_t>( -1))
                        code::raise::error( code::casual::failed_transcoding, "iconv_open - errc: ", code::system::last::error());
                  }

                  ~converter()
                  {
                     posix::log::result( iconv_close( m_descriptor), "iconv_close");
                  }

                  std::string transcode( const std::string& value) const
                  {
                     auto source = const_cast<char*>( value.c_str());
                     auto size = value.size();

                     std::string result;

                     do
                     {
                        char buffer[32];
                        auto left = sizeof buffer;
                        char* target = buffer;

                        const auto conversions = iconv( m_descriptor, &source, &size, &target, &left);

                        if( conversions == std::numeric_limits< decltype( conversions)>::max())
                        {
                           switch( auto code = code::system::last::error())
                           {
                              case std::errc::argument_list_too_long: break;
                              default:
                                 code::raise::error( code::casual::failed_transcoding, "iconv - errc: ", code);
                           }
                        }

                        result.append( buffer, target);

                     }while( size);

                     return result;
                  }

               private:
                  const iconv_t m_descriptor;
               };

               namespace locale::name
               {
                  constexpr auto utf8 = std::string_view{ "UTF-8"};
                  constexpr auto current = std::string_view{ ""};
                  
               } // locale::name

            } // <unnamed>
         } // local

         namespace utf8
         {
            // Make sure this is done once
            //
            // If "" is used as codeset to iconv, it shall choose the current
            // locale-codeset, but that seems to work only if
            // std::setlocale() is called first
            //
            // Perhaps we need to call std::setlocale() each time just in case
            //
            [[maybe_unused]] const auto once = std::setlocale( LC_CTYPE, local::locale::name::current.data());

            std::string encode( const std::string& value)
            {
               return encode( value, local::locale::name::current);
            }

            std::string decode( const std::string& value)
            {
               return decode( value, local::locale::name::current);
            }

            bool exist( std::string_view codeset)
            {
               try
               {
                  local::converter{ codeset, local::locale::name::current};
               }
               catch( ...)
               {
                  if( exception::capture().code() == code::casual::failed_transcoding)
                     return false;

                  throw;
               }

               return true;
            }

            std::string encode( const std::string& value, std::string_view codeset)
            {
               return local::converter( codeset, local::locale::name::utf8).transcode( value);
            }

            std::string decode( const std::string& value, std::string_view codeset)
            {
               return local::converter( local::locale::name::utf8, codeset).transcode( value);
            }

         } // utf8

         namespace hex
         {
            namespace local
            {
               namespace
               {
                  template< typename InIter, typename OutIter>
                  void encode( InIter first, InIter last, OutIter out)
                  {
                     while( first != last)
                     {
                        auto hex = []( auto value){
                           if( value < 10)
                           {
                              return value + 48;
                           }
                           return value + 87;
                        };

                        *out++ = hex( ( 0xf0 & *first) >> 4);
                        *out++ = hex( 0x0f & *first);

                        ++first;
                     }
                  }

                  template< typename InIter, typename OutIter>
                  void decode( InIter first, InIter last, OutIter out)
                  {
                     assert( ( last - first) % 2 == 0);

                     while( first != last)
                     {
                        auto hex = []( decltype( *first) value)
                        {
                           if( value >= 87)
                           {
                              return value - 87;
                           }
                           return value - 48;
                        };

                        *out = ( 0x0F & hex( *first++)) << 4;
                        *out += 0x0F & hex( *first++);

                        ++out;
                     }
                  }

               } // <unnamed>
            } // local

            namespace detail
            {
               std::string encode( const char* first, const char* last)
               {
                  auto bytes = last - first;
                  std::string result( bytes * 2, 0);

                  local::encode( first, last, result.begin());

                  return result;
               }

               void encode( std::ostream& out, const char* first, const char* last)
               {
                  local::encode( first, last, std::ostream_iterator< const char>( out));
               }

               void decode(const char* first, const char* last, char* data)
               {
                  local::decode( first, last, data);
               }
               

            } // detail


            platform::binary::type decode( const std::string& value)
            {
               platform::binary::type result( value.size() / 2);

               local::decode( value.begin(), value.end(), result.begin());

               return result;
            }

         } // hex

      } // transcode
   } // common
} // casual



