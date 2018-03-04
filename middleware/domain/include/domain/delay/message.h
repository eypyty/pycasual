//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_


#include "common/message/type.h"

#include "common/communication/message.h"
#include "common/marshal/complete.h"

#include "common/uuid.h"
#include "common/process.h"

#include <chrono>

namespace casual
{
   namespace domain
   {
      namespace delay
      {
         const common::Uuid& identification();

         namespace message
         {
            struct Request : common::message::basic_message< common::message::Type::delay_message>
            {
               common::strong::ipc::id destination;
               common::platform::time::unit delay;
               common::communication::message::Complete message;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & destination;
                  archive & delay;
                  archive & message;
               })
            };

            void send( const Request& request);


            template< typename M, typename R, typename D>
            void send( M&& message, common::strong::ipc::id destination, std::chrono::duration< R, D> delay)
            {
               Request request;
               request.destination = destination;
               request.delay = std::chrono::duration_cast< common::platform::time::unit>( delay);
               request.message = common::marshal::complete( std::move( message));

               send( request);

            }

         } // message

      } // delay

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DELAY_MESSAGE_H_
