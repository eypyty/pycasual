//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/type.h"
#include "common/cast.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include <map>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace execution::timeout::contract
         {
            Type transform( const std::string& contract)
            {
               if ( contract == "linger") return Type::linger;
               if ( contract == "kill") return Type::kill;
               if ( contract == "terminate") return Type::terminate;
               
               code::raise::error( code::casual::invalid_configuration, "unexpected value: ", contract);
            }

            std::string transform( Type contract)
            {
               std::ostringstream stream{};
               stream << contract;
               return stream.str();
            }

            std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::linger: return out << "linger";
                  case Type::kill: return out << "kill";
                  case Type::terminate: return out << "terminate";
               }
               return out << "unknown";
            }

         }

         namespace transaction
         {
            std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::atomic: return out << "atomic";
                  case Type::join: return out << "join";
                  case Type::automatic: return out << "automatic";
                  case Type::none: return out << "none";
                  case Type::branch: return out << "branch";
               }
               return out << "unknown";
            }

            Type mode( const std::string& mode)
            {
               if( mode == "automatic" || mode == "auto") return Type::automatic;
               if( mode == "join") return Type::join;
               if( mode == "atomic") return Type::atomic;
               if( mode == "none") return Type::none;
               if( mode == "branch") return Type::branch;

               code::raise::error( code::casual::invalid_argument, "transaction mode: ", mode);
            }

            Type mode( std::uint16_t mode)
            {
               if( mode < cast::underlying( Type::automatic) || mode > cast::underlying( Type::branch))
                  code::raise::error( code::casual::invalid_argument, "transaction mode: ", mode);

               return static_cast< Type>( mode);
            }

         } // transaction

      } // service

   } // common


} // casual
