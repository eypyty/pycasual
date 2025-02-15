//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <utility>

namespace casual 
{
   namespace common::predicate
   {
      template< typename... Ps>
      auto make_and( Ps&&... predicates)
      {
         return [=]( auto&&... param){
            return ( ... && predicates( param...));
         };
      }

      template< typename... Ps>
      auto make_or( Ps&&... predicates)
      {
         return [=]( auto&&... param){
            return ( ... || predicates( param...));
         };
      }

      template< typename P>
      auto make_nested( P&& predicate) { return std::forward< P>( predicate);}

      template< typename P, typename... Ts>
      auto make_nested( P&& predicate, Ts&&... ts)
      {
         return [=]( auto&& value){
            return predicate( make_nested( std::move( ts)...)( std::forward< decltype( value)>( value)));
         };
      }

      //! reverse the call order of left and right to a predicate
      template< typename T>
      auto inverse( T&& functor)
      {
         return [functor]( auto&& l, auto&& r){ return functor( r, l);};
      }

      //! negates a predicate
      template< typename T>
      auto negate( T&& functor)
      {
         return [functor]( auto&&... param){ return ! functor( param...);};
      }

      template< typename T>
      auto boolean( T&& value) -> decltype( static_cast< bool>( value))
      {
         return static_cast< bool>( value);
      }

      inline auto boolean()
      {
         return []( auto&& value){ return boolean( value);};
      }

      namespace value
      {
         template< typename T>
         auto equal( T&& wanted)
         {
            return [ wanted = std::forward< T>( wanted)]( auto&& value){ return value == wanted;};
         }
      } // value

      namespace adapter
      {
         //! wraps and invoke `predicate` with `value.second`
         //! useful for iterate over map-like containers. 
         template< typename P>
         auto second( P&& predicate)
         {
            return [ predicate = std::forward< P>( predicate)]( auto& pair)
            {
               return predicate( pair.second); 
            };
         }


         inline auto first()
         {
            return []( auto& pair) -> decltype( pair.first)
            {
               return pair.first;
            };
         }

         inline auto second()
         {
            return []( auto& pair) -> decltype( pair.second)
            {
               return pair.second;
            };
         }
      } // adapter


   } // common::predicate
} // casual 