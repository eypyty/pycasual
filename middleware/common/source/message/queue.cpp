//!
//! queue.cpp
//!
//! Created on: May 2, 2015
//!     Author: Lazan
//!

#include "common/message/queue.h"

#include "common/chronology.h"


namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace queue
         {

            std::ostream& operator << ( std::ostream& out, const base_message& value)
            {
               return out << "{ id: " << value.id
                     << ", type: " << value.type
                     << ", properties: " << value.properties
                     << ", reply: " << value.reply
                     << ", available: " << common::chronology::local( value.avalible)
                     << ", size: " << value.payload.size()
                     << '}';
            }

            namespace lookup
            {
               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ name: " << value.name
                        << '}';

               }
               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
                        << ", queue: " << value.queue
                        << '}';
               }
            } // lookup

            namespace enqueue
            {
               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", trid: " << value.trid
                        << ", queue: " << value.queue
                        << ", name: " << value.name
                        << ", message: " << value.message
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ id: " << value.id
                        << '}';
               }

            } // enqueue

            namespace dequeue
            {
               std::ostream& operator << ( std::ostream& out, const Selector& value)
               {
                  return out << "{ id: " << value.id
                        << ", properties: " << value.properties
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ name: " << value.name
                        << ", queue: " << value.queue
                        << ", block: " << std::boolalpha << value.block
                        << ", selector: " << value.selector
                        << ", process: " << value.process
                        << ", trid: " << value.trid << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ message: " << range::make( value.message)
                        << '}';
               }

               namespace forget
               {

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", process: " << value.process
                        << ", queue: " << value.queue
                        << ", name: " << value.name << '}';
                  }


                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ correlation: " << value.correlation
                        << ", found: " << value.found << '}';
                  }

               } // forget
            } // dequeue

            namespace peek
            {
               std::ostream& operator << ( std::ostream& out, const Information& value)
               {
                  return out << "{"
                        << '}';
               }

               namespace information
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{"
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{"
                           << '}';
                  }
               } // information

               namespace messages
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{"
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{"
                           << '}';
                  }
               } // messages

            } // peek

            std::ostream& operator << ( std::ostream& out, const Queue::Type& value)
            {
               switch( value)
               {
                  case Queue::Type::group_error_queue: { return out << "group-error-queue";}
                  case Queue::Type::error_queue: { return out << "error-queue";}
                  case Queue::Type::queue: { return out << "queue";}
               }
               return out << "unknown";
            }

            std::ostream& operator << ( std::ostream& out, const Queue& value)
            {
               return out << "{ queue: " << value.id
                     << ", name: " << value.name
                     << ", type: " << value.type
                     << ", retries: " << value.retries
                     << ", error: " << value.error
                     << '}';

            }

         } // queue
      } // message
   } // common
} // casual
