//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/value.h"
#include "common/serialize/archive.h"
#include "common/stream/customization.h"
#include "common/cast.h"


#include <ostream>

namespace casual
{
   namespace common::serialize::line
   {
      namespace detail
      {
         constexpr auto first = "";
         constexpr auto scope = ", ";
         
         template<typename T, typename C = traits::remove_cvref_t< T>>
         auto write( std::ostream& out, T&& value, traits::priority::tag< 2>)
            -> decltype( stream::customization::supersede::point< C>::stream( out, std::forward< T>( value)))
         {
            stream::customization::supersede::point< C>::stream( out, std::forward< T>( value));
         }

         template<typename T>
         auto write( std::ostream& out, T&& value, traits::priority::tag< 1>)
            -> decltype( void( out << std::forward< T>( value)))
         {
            out << std::forward< T>( value);
         }

         template<typename T, typename C = traits::remove_cvref_t< T>>
         auto write( std::ostream& out, T&& value, traits::priority::tag< 0>)
            -> decltype( stream::customization::point< C >::stream( out, std::forward< T>( value)))
         {
            stream::customization::point< C >::stream( out, std::forward< T>( value));
         }

         //! to enable to override std ostream stream operatator, and use our specializations
         template<typename T>
         auto write( std::ostream& out, T&& value)
            -> decltype( write( out, std::forward< T>( value), traits::priority::tag< 2>{}))
         {
            write( out, std::forward< T>( value), traits::priority::tag< 2>{});
         }
         
      } // detail

      struct Writer
      {
         Writer();
         
         inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

         static std::vector< std::string> keys();

         platform::size::type container_start( const platform::size::type size, const char* name);
         void container_end( const char*);

         void composite_start( const char* name);
         void composite_end( const char*);

         template<typename T>
         auto write( T&& value, const char* name, common::traits::priority::tag< 1>) 
            -> std::enable_if_t< traits::is::archive::write::type_v< common::traits::remove_cvref_t< T>>>
         {
            save( std::forward< T>( value), name);
         }

         template< typename T>
         auto write( T&& value, const char* name, common::traits::priority::tag< 0>) 
            -> decltype( detail::write( std::declval< std::ostream&>(), std::forward< T>( value)))
         {
            detail::write( maybe_name( name), std::forward< T>( value));
         }

         template<typename T>
         auto write( T&& value, const char* name)
            -> decltype( write( std::forward< T>( value), name, common::traits::priority::tag< 1>{}))
         {
            in_scope();
            write( std::forward< T>( value), name, common::traits::priority::tag< 1>{});
         }


         template< typename T>
         auto operator << ( T&& value)
            -> decltype( void( serialize::value::write( *this, std::forward< T>( value), nullptr)), *this)
         {
            serialize::value::write( *this, std::forward< T>( value), nullptr);
            return *this;
         }
         
         void consume( std::ostream& destination);
         std::string consume();

      private:


         template< typename T>
         auto save( T value, const char* name) -> std::enable_if_t< std::is_arithmetic_v< T>>
         {
             maybe_name( name) << value;
         }

         void save( bool value, const char* name);
         void save( view::immutable::Binary value, const char* name);
         void save( const platform::binary::type& value, const char* name);
         void save( const std::string& value, const char* name);
         void save( const string::immutable::utf8& value, const char* name);

         std::ostream& maybe_name( const char* name);

         void begin_scope();
         void in_scope();

         std::ostringstream m_stream;
         const char* m_prefix = detail::first;
      };

      //! type erased line writer 
      serialize::Writer writer();


   } // common::serialize::line
} // casual




