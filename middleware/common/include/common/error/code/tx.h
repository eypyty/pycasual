//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_TX_H_
#define CASUAL_COMMON_ERROR_CODE_TX_H_

#include "tx/code.h"
#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {
            
            enum class tx : int
            {
               not_supported = TX_NOT_SUPPORTED,
               ok = TX_OK,
               outside = TX_OUTSIDE,
               rollback = TX_ROLLBACK,
               mixed = TX_MIXED,
               hazard = TX_HAZARD,
               protocol = TX_PROTOCOL_ERROR,
               error = TX_ERROR,
               fail = TX_FAIL,
               argument = TX_EINVAL,
               committed = TX_COMMITTED,
               no_begin = TX_NO_BEGIN,

               no_begin_rollback = TX_ROLLBACK_NO_BEGIN,
               no_begin_mixed = TX_MIXED_NO_BEGIN,
               no_begin_hazard = TX_HAZARD_NO_BEGIN,
               no_begin_committed = TX_COMMITTED_NO_BEGIN,
            };

            static_assert( static_cast< int>( tx::ok) == 0, "tx::ok has to be 0");

            std::error_code make_error_code( tx code);

            common::log::Stream& stream( code::tx code);
            

         } // code 
      } // error

      namespace exception
      {
         namespace tx
         {
            error::code::tx handler();
         } // tx
      } // exception

   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::error::code::tx> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {
            inline std::ostream& operator << ( std::ostream& out, code::tx value) { return out << std::error_code( value);}
         } // code 
      } // error
   } // common
} // casual

#endif