//!
//! service.cpp
//!
//! casual
//!

#include "common/server/service.h"

#include "common/algorithm.h"



#include <map>

namespace casual
{
   namespace common
   {
      namespace server
      {


         Service::Service( std::string name, function_type function, std::string category, service::transaction::Type transaction)
            : origin( std::move( name)), function( function), category( std::move( category)), transaction( transaction) {}

         Service::Service( std::string name, function_type function)
            : Service( std::move( name), std::move( function), "", service::transaction::Type::automatic) {}


         Service::Service( Service&&) = default;
         Service& Service::operator = ( Service&&) = default;



         void Service::call( TPSVCINFO* serviceInformation)
         {
            function( serviceInformation);
         }

         Service::target_type Service::adress() const
         {
            return function.target<void(*)(TPSVCINFO*)>();
         }


         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ origin: " << service.origin
                  << ", category: " << service.category
                  << ", transaction: " << service.transaction
                  << '}';
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            if( lhs.adress() && rhs.adress())
               return *lhs.adress() == *rhs.adress();

            return lhs.adress() == rhs.adress();
         }

         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return ! ( lhs == rhs);
         }

      } // server
   } // common
} // casual
