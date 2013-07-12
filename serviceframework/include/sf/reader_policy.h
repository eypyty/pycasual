//!
//! casual_reader_policy.h
//!
//! Created on: Nov 17, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_READER_POLICY_H_
#define CASUAL_READER_POLICY_H_

#include "sf/exception.h"

namespace casual
{
   namespace sf
   {
      namespace policy
      {
         namespace reader
         {
            class Strict
            {
            public:

               inline void initalization()
               {
                  // no op
               }

               template< typename T>
               inline void value( const char* role, T& value) const
               {
                  validationError( role);
               }

               inline void serialtype( const char* role) const
               {
                  validationError( role);
               }

               inline std::size_t container( const char* role) const
               {
                  validationError( role);
                  return 0;
               }



            private:

               void validationError( const std::string& role) const
               {
                  throw exception::Validation( "could not extract '" + role + "'");
               }

            };

            class Relaxed
            {
            public:
               inline void initalization()
               {
                  throw exception::Validation( "could not initialize archive'");
               }

               template< typename T>
               inline void value( const char* role, T& value) const
               {
                  //value = T();
               }

               inline void serialtype( const char* role) const
               {
                  // no op
               }

               inline std::size_t container( const char* role) const
               {
                  return 0;
               }
            };



         }

      }

   }

}



#endif /* CASUAL_READER_POLICY_H_ */
