//!
//! casual
//!

#include "common/message/service.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace local
         {
            namespace
            {
               namespace output
               {
                  void base_service( std::ostream& out, const service::Base& value)
                  {
                     out << "name: " << value.name
                           << ", category: " << value.category
                           << ", mode: " << value.transaction;
                  }
               } // output
            } // <unnamed>
         } // local

         namespace service
         {
            std::ostream& operator << ( std::ostream& out, const Base& value)
            {
               out << "{ ";
               local::output::base_service( out, value);
               return out << '}';
            }
         } // service
         std::ostream& operator << ( std::ostream& out, const Service& value)
         {
            out << "{ ";
            local::output::base_service( out, value);
            return out << ", timeout: " << value.timeout.count() << '}';
         }

         namespace service
         {
            namespace call
            {
               std::ostream& operator << ( std::ostream& out, const call::Service& value)
               {
                  out << "{ ";
                  local::output::base_service( out, value);
                  return out << ", timeout: " << value.timeout.count()
                        << ", event_subscribers: " << range::make( value.event_subscribers)
                        << '}';
               }
            } // call

            std::ostream& operator << ( std::ostream& out, const Transaction& message)
            {
               return out << "{ trid: " << message.trid
                     << ", state: " << message.state
                     << '}';
            }


            std::ostream& operator << ( std::ostream& out, Advertise::Directive value)
            {
               switch( value)
               {
                  case Advertise::Directive::add: return out << "add";
                  case Advertise::Directive::remove: return out << "remove";
                  case Advertise::Directive::replace: return out << "replace";
               }
               return out << "unknown";
            }

            std::ostream& operator << ( std::ostream& out, const Advertise& message)
            {
               return out << "{ process: " << message.process
                     << ", directive: " << message.directive
                     << ", services: " << range::make( message.services)
                     << '}';
            }

            namespace lookup
            {
               std::ostream& operator << ( std::ostream& out, const Request::Context& value)
               {
                  switch( value)
                  {
                     case Request::Context::forward: return out << "forward";
                     case Request::Context::gateway: return out << "gateway";
                     case Request::Context::no_reply: return out << "no_reply";
                     case Request::Context::regular: return out << "regular";
                  }
                  return out << "unknown";
               }

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ process: " << value.process
                        << ", requested: " << value.requested
                        << ", context: " << value.context
                        << '}';

               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  out << "{ service: " << value.service << ", process: " << value.process << ", state: ";
                  switch( value.state)
                  {
                     case Reply::State::absent: out << "absent"; break;
                     case Reply::State::idle: out << "idle"; break;
                     case Reply::State::busy: out << "busy"; break;
                  }

                  return out << '}';
               }

            } // lookup

            namespace call
            {

               std::ostream& operator << ( std::ostream& out, const common_request& value)
               {
                  auto& header = common::service::header::fields();

                  return out << "{ process: " << value.process
                     << ", service: " << value.service
                     << ", parent: " << value.parent
                     << ", flags: " << value.flags
                     << ", header: " << range::make( header)
                     << '}';
               }


               std::ostream& operator << ( std::ostream& out, const Reply& message)
               {
                  return out << "{ transaction: " << message.transaction
                        << ", error: " << message.error
                        << ", code: " << message.code
                        << ", buffer: " << message.buffer
                        << '}';
               }

            } // call
         }
      } // message
   } // common
} // casual
