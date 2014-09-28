//!
//! casual_queue.h
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "common/ipc.h"
#include "common/message/type.h"
#include "common/marshal.h"
#include "common/process.h"


#include <list>

namespace casual
{
   namespace common
   {

      namespace queue
      {
         typedef ipc::message::Transport transport_type;
         typedef transport_type::message_type_type message_type_type;


         namespace policy
         {
            struct NoAction
            {
               void apply()
               {
                  throw;
               }
            };

            template< typename S>
            struct RemoveOnTerminate
            {
               using state_type = S;

               RemoveOnTerminate( state_type& state) : m_state( state) {}

               void apply()
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::signal::child::Terminate& exception)
                  {
                     auto terminated = process::lifetime::ended();
                     for( auto& death : terminated)
                     {
                        switch( death.reason)
                        {
                           case process::lifetime::Exit::Reason::core:
                              log::error << death << std::endl;
                              break;
                           default:
                              log::internal::debug << death << std::endl;
                              break;
                        }

                        m_state.removeProcess( death.pid);
                     }
                  }
               }

            protected:
               state_type& m_state;
            };


            struct Blocking
            {
               enum Flags
               {
                  flags = 0
               };


               template< typename M>
               static void send( M&& message, ipc::send::Queue& ipc)
               {
                  ipc( std::forward< M>( message), flags);
               }

               static marshal::input::Binary next( ipc::receive::Queue& ipc);

               static marshal::input::Binary next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types);

               template< typename M>
               static void fetch( ipc::receive::Queue& ipc, M& message)
               {
                  auto type = message::type( message);

                  auto transport = ipc( type, flags);

                  assert( ! transport.empty());

                  marshal::input::Binary marshal( std::move( transport.front()));
                  marshal >> message;
               }

            };

            struct NonBlocking
            {
               enum Flags
               {
                  flags = ipc::receive::Queue::cNoBlocking
               };

               template< typename M>
               static bool send( M&& message, ipc::send::Queue& ipc)
               {
                  return ipc( std::forward< M>( message), flags);
               }

               static std::vector< marshal::input::Binary> next( ipc::receive::Queue& ipc);

               static std::vector< marshal::input::Binary> next( ipc::receive::Queue& ipc, const std::vector< platform::message_type_type>& types);


               template< typename M>
               static bool fetch( ipc::receive::Queue& ipc, M& message)
               {
                  auto type = message::type( message);

                  auto transport = ipc( type, flags);

                  if( ! transport.empty())
                  {
                     marshal::input::Binary marshal( std::move( transport.front()));
                     marshal >> message;
                     return true;
                  }

                  return false;
               }

            };


         } // policy

         namespace internal
         {


            template< typename BP, typename P>
            struct basic_writer
            {
            public:
               typedef BP block_policy;
               typedef P policy_type;


               template< typename... Args>
               basic_writer( platform::queue_id_type id, Args&&... args)
                  : m_ipc( id), m_policy( std::forward< Args>( args)...) {}


               //!
               //! Sends/Writes a message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @note non-blocking
               //! @return true if the whole message is sent. false otherwise
               //!
               template< typename M>
               auto operator () ( M&& message) -> decltype( block_policy::send( message, std::declval< ipc::send::Queue&>()))
               {
                  auto transport = prepare( std::forward< M>( message));

                  return send( transport);
               }

               const ipc::send::Queue& ipc() const
               {
                  return m_ipc;
               }


               //!
               //! Sends/Writes a "complete" message to the queue. which can result in several
               //! actual ipc-messages.
               //!
               //! @return depending on block_policy, if blocking void, if non-blocking  true if message is sent, false otherwise
               //!
               auto send( const ipc::message::Complete& transport) -> decltype( block_policy::send( transport, std::declval< ipc::send::Queue&>()))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::send( transport, m_ipc);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

            private:

               template< typename M>
               static ipc::message::Complete prepare( M&& message)
               {
                  //
                  // Serialize the message
                  //
                  marshal::output::Binary archive;
                  archive << message;

                  auto type = message::type( message);
                  return ipc::message::Complete( type, archive.release());
               }

               ipc::send::Queue m_ipc;
               policy_type m_policy;

            };



            template< typename BP, typename P>
            class basic_reader
            {
            public:

               typedef BP block_policy;
               typedef P policy_type;
               //typedef IPC ipc_value_type;
               //using ipc_type = typename std::decay< ipc_value_type>::type;
               using type_type = platform::message_type_type;

               template< typename... Args>
               basic_reader( ipc::receive::Queue& ipc, Args&&... args)
                  : m_ipc( ipc), m_policy( std::forward< Args>( args)...) {}

               //!
               //! Gets the next binary-message from queue.
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next() -> decltype( block_policy::next( std::declval< ipc::receive::Queue&>()))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::next( m_ipc);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }


               //!
               //! Gets the next binary-message from queue that is any of @p types
               //! @return binary-marshal that can be used to deserialize an actual message.
               //!
               auto next( const std::vector< type_type>& types) -> decltype( block_policy::next( std::declval< ipc::receive::Queue&>(), { type_type()}))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::next( m_ipc, types);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }


               //!
               //! Tries to read a specific message from the queue.
               //! If other message types is consumed before the requested type
               //! they will be cached.
               //!
               //! @attention Will block until the specific message-type can be read from the queue
               //!
               template< typename M>
               auto operator () ( M& message) -> decltype( block_policy::fetch( std::declval< ipc::receive::Queue&>(), message))
               {
                  while( true)
                  {
                     try
                     {
                        return block_policy::fetch( m_ipc, message);
                     }
                     catch( ...)
                     {
                        m_policy.apply();
                     }
                  }
               }

               const ipc::receive::Queue& ipc() const
               {
                  return m_ipc;
               }

            private:

               ipc::receive::Queue& m_ipc;
               policy_type m_policy;
            };



         } // internal


         namespace blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::Blocking, P>;

            typedef basic_writer< policy::NoAction> Writer;


            template< typename P>
            using basic_reader = internal::basic_reader< policy::Blocking, P>;

            typedef basic_reader< policy::NoAction> Reader;

            inline Reader reader( ipc::receive::Queue& ipc)
            {
               return Reader( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_writer = basic_writer< policy::RemoveOnTerminate< S>>;

               template< typename S>
               using basic_reader = basic_reader< policy::RemoveOnTerminate< S>>;
            }

         } // blocking

         namespace non_blocking
         {

            template< typename P>
            using basic_writer = internal::basic_writer< policy::NonBlocking, P>;

            typedef basic_writer< policy::NoAction> Writer;

            template< typename P>
            using basic_reader = internal::basic_reader< policy::NonBlocking, P>;

            typedef basic_reader< policy::NoAction> Reader;


            inline Reader reader( ipc::receive::Queue& ipc)
            {
               return Reader( ipc);
            }

            namespace remove
            {
               template< typename S>
               using basic_writer = basic_writer< policy::RemoveOnTerminate< S>>;

               template< typename S>
               using basic_reader = basic_reader< policy::RemoveOnTerminate< S>>;
            }


         } // non_blocking

         template< typename Q>
         struct is_blocking : public std::integral_constant< bool, std::is_same< typename Q::block_policy, policy::Blocking>::value>
         {

         };


      } // queue
   } // common
} // casual


#endif /* CASUAL_QUEUE_H_ */
