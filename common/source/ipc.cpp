//!
//! casual_ipc.cpp
//!
//! Created on: Mar 27, 2012
//!     Author: Lazan
//!

#include "common/ipc.h"

#include "common/environment.h"
#include "common/process.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/signal.h"
#include "common/uuid.h"
#include "common/internal/log.h"
#include "common/algorithm.h"


#include <fstream>

#include <cstdio>
#include <unistd.h>
#include <errno.h>
#include <signal.h>


// temp
#include <iostream>




namespace casual
{
   namespace common
   {
      namespace ipc
      {
         namespace internal
         {

            base_queue::id_type base_queue::id() const
            {
               return m_id;
            }


         } // internal


         namespace message
         {

            Complete::Complete( Transport& transport)
               : type( transport.m_payload.m_type), correlation( transport.m_payload.m_header.m_correlation)
            {
               add( transport);
            }

            void Complete::add( Transport& transport)
            {
               payload.insert(
                  std::end( payload),
                  std::begin( transport.m_payload.m_payload),
                  std::begin( transport.m_payload.m_payload) + transport.paylodSize());

               complete = transport.m_payload.m_header.m_count == 0;

            }
         }// message


         namespace send
         {
            Queue::Queue( id_type id) : internal::base_queue( id)
            {

            }

            bool Queue::send( const message::Complete& message, const long flags) const
            {
               //
               // partition the payload and send the resulting physical messages
               //

               ipc::message::Transport transport;
               transport.m_payload.m_type = message.type;
               message.correlation.copy( transport.m_payload.m_header.m_correlation);

               //
               // This crap got to do a lot better...
               //
               auto size = message.payload.size();
               if( size % message::Transport::payload_max_size == 0 && size > 0)
               {
                  transport.m_payload.m_header.m_count = size / message::Transport::payload_max_size - 1;
               }
               else
               {
                  transport.m_payload.m_header.m_count = size / message::Transport::payload_max_size;
               }


               auto partBegin = std::begin( message.payload);

               while( partBegin != std::end( message.payload))
               {

                  auto partEnd = partBegin + message::Transport::payload_max_size > std::end( message.payload) ?
                        std::end( message.payload) : partBegin + message::Transport::payload_max_size;

                  std::copy( partBegin, partEnd, std::begin( transport.m_payload.m_payload));

                  transport.paylodSize( partEnd - partBegin);

                  //
                  // send the physical message
                  //
                  if( ! send( transport, flags))
                  {
                     if( transport.m_payload.m_header.m_count > 0)
                     {
                        // TODO: Partially sent message, what to do now?
                        log::internal::ipc << "TODO: Partially sent message, what to do now?\n";
                     }
                     return false;
                  }

                  --transport.m_payload.m_header.m_count;


                  partBegin = partEnd;
               }

               log::internal::ipc << "sent message - type: " << message.type <<  " size: " << message.payload.size() << " destination: " << m_id << " correlation: " << message.correlation << std::endl;

               return true;
            }

            bool Queue::send( message::Transport& message, const long flags) const
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();
               auto result = msgsnd( m_id, message.raw(), message.size(), flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        common::signal::handle();
                        return false;
                     }
                     case EAGAIN:
                     {
                        return false;
                     }
                     default:
                     {
                        throw common::exception::QueueSend( "id: " + std::to_string( m_id) + " - " + common::error::string());
                     }
                  }
               }
               return true;
            }

         } // send

         namespace receive
         {

            Queue::Queue()
               : internal::base_queue( msgget( IPC_PRIVATE, IPC_CREAT | 0660)),
                    m_scopedPath( environment::directory::temporary() + "/ipc_queue_" + Uuid::make().string())
            {
               if( m_id  == -1)
               {
                  throw common::exception::QueueFailed( common::error::string());
               }

               //
               // Write queue information
               //
               std::ofstream ipcQueueFile( m_scopedPath.path());

               ipcQueueFile << "id: " << m_id << std::endl
                     << "pid: " << process::id() <<  std::endl
                     << "path: " << process::path() << std::endl;


               log::internal::ipc << "created queue - id: " << m_id << " pid: " <<  process::id() << std::endl;

            }


            Queue::~Queue()
            {
               if( ! m_cache.empty())
               {
                  log::error << "queue: " << m_id << " has unconsumed messages in cache";
               }

               try
               {
                  //
                  // Destroy queue
                  //
                  ipc::remove( m_id);
                  log::internal::ipc << "queue id: " << m_id << " removed" << std::endl;
               }
               catch( ...)
               {
                  error::handler();
               }
            }


            namespace local
            {
               namespace
               {
                  namespace find
                  {
                     struct Complete
                     {
                        bool operator ()( const message::Complete& message) const
                        {
                           return message.complete;
                        }
                     };


                     struct Type
                     {
                        typedef message::Complete::message_type_type message_type;
                        Type( message_type type) : m_type( type) {}

                        bool operator () ( const message::Complete& message) const
                        {
                           return message.type == m_type;
                        }


                     private:
                        message_type m_type;

                     };

                     struct Types
                     {
                        typedef message::Complete::message_type_type message_type;
                        using message_types = std::vector< message_type>;

                        Types( message_types types) : m_types( std::move( types)) {}

                        bool operator () ( const message::Complete& message) const
                        {
                           return ! range::find( m_types, message.type).empty();
                        }


                     private:
                        message_types m_types;

                     };


                     struct Correlation
                     {
                        Correlation( message::Transport::correalation_type& correlation) : m_correlation( correlation) {}

                        bool operator ()( const message::Complete& complete) const
                        {
                           return complete.correlation == m_correlation;
                        }
                     private:
                        message::Transport::correalation_type& m_correlation;
                     };
                  } // find
               } // <unnamed>
            } // local




            std::vector< message::Complete> Queue::operator () ( const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find( local::find::Complete(), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "received message - type: " << result.back().type << " size: " << result.back().payload.size() << " correlation: " <<  result.back().correlation << std::endl;
               }

               return result;
            }

            std::vector< message::Complete> Queue::operator () ( message::Complete::message_type_type type, const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find(
                     chain::And::link(
                           local::find::Type( type),
                           local::find::Complete()), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "received message - type: " << result.back().type << " size: " << result.back().payload.size() << " correlation: " <<  result.back().correlation << std::endl;
               }

               return result;
            }

            std::vector< message::Complete> Queue::operator () ( const std::vector< type_type>& types, const long flags)
            {
               std::vector< message::Complete> result;

               auto found = find(
                     chain::And::link(
                           local::find::Types( types),
                           local::find::Complete()), flags);

               if( found)
               {
                  result.push_back( std::move( *found));
                  m_cache.erase( found.first);

                  log::internal::ipc << "received message - type: " << result.back().type << " size: " << result.back().payload.size() << " correlation: " <<  result.back().correlation << std::endl;
               }

               return result;
            }


            template< typename P>
            Queue::range_type Queue::find( P predicate, const long flags)
            {
               auto found = range::find_if( m_cache, predicate);

               message::Transport transport;

               while( ! found && receive( transport, flags))
               {
                  found = cache( transport);
                  found = range::find_if( found, predicate);
               }

               return found;
            }


            bool Queue::receive( message::Transport& message, const long flags)
            {
               //
               // We have to check and process (throw) pending signals before we might block
               //
               common::signal::handle();
               auto result = msgrcv( m_id, message.raw(), message.size(), 0, flags);

               if( result == -1)
               {
                  switch( errno)
                  {
                     case EINTR:
                     {
                        common::signal::handle();
                        return false;
                     }
                     case ENOMSG:
                     {
                        return false;
                     }
                     default:
                     {
                        throw common::exception::QueueReceive( common::error::string() + " - id: " + std::to_string( m_id) + " - flags: " + std::to_string( flags) + " - size: " + std::to_string( message.size()));
                     }
                  }
               }

               message.size( result);

               return result > 0;
            }

            Queue::range_type Queue::cache( message::Transport& message)
            {
               auto found = range::find_if( m_cache,
                     local::find::Correlation( message.m_payload.m_header.m_correlation));

               if( found)
               {
                  found->add( message);
               }
               else
               {
                  found.first = m_cache.emplace( std::end( m_cache), message);
                  found.last = std::end( m_cache);
               }
               return found;
            }

         } // receive

         namespace local
         {
            namespace
            {
               send::Queue initializeBrokerQueue()
               {
                  static const std::string brokerFile = common::environment::file::brokerQueue();

                  std::ifstream file( brokerFile.c_str());

                  if( file.fail())
                  {
                     log::internal::ipc << "Failed to open broker queue configuration file" << std::endl;
                     throw common::exception::xatmi::SystemError( "Failed to open broker queue configuration file: " + brokerFile);
                  }

                  send::Queue::id_type id{ 0};
                  file >> id;

                  return send::Queue( id);

               }
            }
         }

         namespace broker
         {


            send::Queue& queue()
            {
               static send::Queue brokerQueue = local::initializeBrokerQueue();
               return brokerQueue;
            }

            send::Queue::id_type id()
            {
               return queue().id();
            }

         } // broker


         namespace receive
         {
            receive::Queue::id_type id()
            {
               return queue().id();
            }

            receive::Queue& queue()
            {
               static receive::Queue singleton;
               return singleton;
            }

         } // receive

         send::Queue& getBrokerQueue()
         {
            return broker::queue();
         }

         receive::Queue& getReceiveQueue()
         {
            return receive::queue();
         }

         void remove( platform::queue_id_type id)
         {
            if( msgctl( id, IPC_RMID, nullptr) != 0)
            {
               throw exception::NotReallySureWhatToNameThisException( error::string());
            }
         }

      } // ipc
	} // common
} // casual

