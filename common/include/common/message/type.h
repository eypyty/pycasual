//!
//! type.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMONMESSAGETYPE_H_
#define COMMONMESSAGETYPE_H_

#include "common/platform.h"
#include "common/transaction/id.h"

#include "common/marshal/marshal.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         enum Type
         {

            UTILITY_BASE = 500,
            cFlushIPC, // dummy message used to flush queue (into cache)
            cPoke,
            cShutdowndRequest,
            cShutdowndReply,
            cForwardConnectRequest,
            cForwardConnectReply,
            cProcessDeathRegistration,
            cProcessDeathEvent,

            // Server
            SERVER_BASE = 1000, // message type can't be 0!
            cServerConnectRequest,
            cServerConnectReply,
            cServerDisconnect,
            cServerPingRequest,
            cServerPingReply,

            // Service
            SERVICE_BASE = 2000,
            cServiceAdvertise,
            cServiceUnadvertise,
            cServiceNameLookupRequest,
            cServiceNameLookupReply,
            cServiceCall,
            cServiceReply,
            cServiceAcknowledge,

            // Monitor
            TRAFFICMONITOR_BASE = 3000,
            cTrafficMonitorConnectRequest,
            cTrafficMonitorConnectReply,
            cTrafficMonitorDisconnect,
            cTrafficEvent,

            // Transaction
            TRANSACTION_BASE = 4000,
            cTransactionClientConnectRequest,
            cTransactionClientConnectReply,
            cTransactionManagerConnectRequest,
            cTransactionManagerConnectReply,
            cTransactionManagerConfiguration,
            cTransactionManagerReady,
            cTransactionBeginRequest = 4100,
            cTransactionBeginReply,
            cTransactionCommitRequest,
            cTransactionCommitReply,
            cTransactionRollbackRequest,
            cTransactionRollbackReply,
            cTransactionGenericReply,

            cTransactionResurceConnectReply = 4200,
            cTransactionResourcePrepareRequest,
            cTransactionResourcePrepareReply,
            cTransactionResourceCommitRequest,
            cTransactionResourceCommitReply,
            cTransactionResourceRollbackRequest,
            cTransactionResourceRollbackReply,
            cTransactionDomainResourcePrepareRequest = 4300,
            cTransactionDomainResourcePrepareReply,
            cTransactionDomainResourceCommitRequest,
            cTransactionDomainResourceCommitReply,
            cTransactionDomainResourceRollbackRequest,
            cTransactionDomainResourceRollbackReply,
            cTransactionResourceInvolved = 4400,

            // casual queue
            QUEUE_BASE = 5000,
            cQueueConnectRequest,
            cQueueConnectReply,
            cQueueEnqueueRequest = 5100,
            cQueueEnqueueReply,
            cQueueDequeueRequest = 5200,
            cQueueDequeueReply,
            cQueueDequeueForgetRequest,
            cQueueDequeueForgetReply,
            cQueueInformation = 5300,
            cQueueQueuesInformationRequest,
            cQueueQueuesInformationReply,
            cQueueQueueInformationRequest,
            cQueueQueueInformationReply,
            cQueueLookupRequest = 5400,
            cQueueLookupReply,
            cQueueGroupInvolved = 5500,
         };

         template< message::Type type>
         struct basic_message
         {

            using base_type = basic_message< type>;

            enum
            {
               message_type = type
            };

            Uuid correlation;

            //!
            //! The execution-id
            //!
            mutable Uuid execution;

            CASUAL_CONST_CORRECT_MARSHAL(
            {

               //
               // correlation is part of ipc::message::Complete, and is
               // handled by the ipc-abstraction (marshaled 'on the side')
               //

               archive & execution;
            })
         };

         template< typename M, message::Type type>
         struct type_wrapper : public M
         {
            using M::M;

            enum
            {
               message_type = type
            };

            Uuid correlation;

            //!
            //! The execution-id
            //!
            mutable Uuid execution;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               //
               // correlation is part of ipc::message::Complete, and is
               // handled by the ipc-abstraction (marshaled 'on the side')
               //

               archive & execution;
               M::marshal( archive);
            })
         };

         namespace flush
         {
            using IPC = basic_message< cFlushIPC>;

         } // flush




         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         constexpr platform::message_type_type type( const M&)
         {
            return M::message_type;
         }

         template< typename M>
         auto correlation( M& message) -> decltype( message.correlation)
         {
            return message.correlation;
         }


         //!
         //! Message to "force" exit/termination.
         //! useful in unittest, to force exit on blocking read
         //!
         namespace shutdown
         {
            struct Request : basic_message< cShutdowndRequest>
            {
               process::Handle process;
               bool reply = false;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & reply;
               })
            };

            struct Reply : basic_message< cShutdowndReply>
            {
               template< typename ID>
               struct holder_t
               {
                  std::vector< ID> online;
                  std::vector< ID> offline;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     archive & online;
                     archive & offline;
                  })
               };

               holder_t< platform::pid_type> executables;
               holder_t< process::Handle> servers;


               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  base_type::marshal( archive);
                  archive & executables;
                  archive & servers;
               })
            };

         } // shutdown



         //
         // Below, some basic message related types that is used by others
         //

         struct Service
         {

            Service() = default;
            Service& operator = (const Service& rhs) = default;



            explicit Service( std::string name, std::uint64_t type, std::uint64_t transaction)
               : name( std::move( name)), type( type), transaction( transaction)
            {}

            Service( std::string name)
               : Service( std::move( name), 0, 0)
            {}

            std::string name;
            std::uint64_t type = 0;
            std::chrono::microseconds timeout = std::chrono::microseconds::zero();
            std::vector< platform::queue_id_type> traffic_monitors;
            std::uint64_t transaction = 0;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & name;
               archive & type;
               archive & timeout;
               archive & traffic_monitors;
               archive & transaction;
            })

            friend std::ostream& operator << ( std::ostream& out, const Service& value);
         };

         namespace server
         {

            template< message::Type type>
            struct basic_id : basic_message< type>
            {
               process::Handle process;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  basic_message< type>::marshal( archive);
                  archive & process;
               })
            };

            namespace connect
            {

               template< message::Type type>
               struct basic_request : basic_id< type>
               {

                  std::string path;
                  Uuid identification;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_id< type>::marshal( archive);
                     archive & path;
                     archive & identification;
                  })
               };

               template< message::Type type>
               struct basic_reply : basic_message< type>
               {
                  enum class Directive : char
                  {
                     start,
                     singleton,
                     shutdown
                  };

                  Directive directive = Directive::start;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< type>::marshal( archive);
                     archive & directive;
                  })
               };
            } // connect



            template< message::Type type>
            struct basic_disconnect : basic_id< type>
            {

            };

         } // server

         namespace dead
         {
            namespace process
            {
               using Registration = server::basic_id< cProcessDeathRegistration>;

               struct Event : basic_message< cProcessDeathEvent>
               {
                  Event() = default;
                  Event( common::process::lifetime::Exit death) : death{ std::move( death)} {}

                  common::process::lifetime::Exit death;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_message< cProcessDeathEvent>::marshal( archive);
                     archive & death;
                  })
               };

            } // process
         } // dead

         namespace forward
         {
            namespace connect
            {
               using Request = server::connect::basic_request< cForwardConnectRequest>;
               using Reply = server::connect::basic_reply< cForwardConnectReply>;
            } // connect

         } // forward

         namespace reverse
         {


            //!
            //! declaration of helper tratis to get the
            //! "reverse type". Normally to get the Reply-type
            //! from a Request-type, and vice versa.
            //!
            template< typename T>
            struct type_traits;

            namespace detail
            {
               template< typename R>
               struct type
               {
                  using reverse_type = R;

               };
            } // detail

            template<>
            struct type_traits< shutdown::Request> : detail::type< shutdown::Reply> {};

            template<>
            struct type_traits< forward::connect::Request> : detail::type< forward::connect::Reply> {};


            template< typename T>
            auto type( T&& message) -> typename type_traits< typename std::decay<T>::type>::reverse_type
            {
               typename type_traits< typename std::decay<T>::type>::reverse_type result;

               result.correlation = message.correlation;
               result.execution = message.execution;

               return result;
            }




         } // reverse

      } // message
   } // common
} // casual

#endif // TYPE_H_
