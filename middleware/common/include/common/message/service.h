//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/message/type.h"
#include "common/message/event.h"

#include "common/transaction/id.h"
#include "common/service/type.h"
#include "common/buffer/type.h"
#include "common/uuid.h"
#include "common/flag/xatmi.h"
#include "common/code/xatmi.h"

#include "common/service/header.h"

#include "common/serialize/line.h"

namespace casual
{
   namespace common::message
   {
      namespace service
      {
         struct Code 
         {
            code::xatmi result = code::xatmi::ok;
            long user{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( result);
               CASUAL_SERIALIZE( user);
            )
         };

         enum class Type : short
         {
            sequential = 1,
            concurrent = 2,
         };

         struct Base
         {
            Base() = default;

            explicit Base( std::string name, std::string category, common::service::transaction::Type transaction)
               : name( std::move( name)), category( std::move( category)), transaction( transaction)
            {}

            Base( std::string name)
               : name( std::move( name))
            {}

            std::string name;
            std::string category;
            common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
            service::Type type = service::Type::sequential;

            inline friend bool operator == ( const Base& lhs, const std::string& rhs) { return lhs.name == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( category);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( type);
            )
         };

         struct Timeout
         {
            platform::time::unit duration{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( duration);
            )

         };

      } // service

      struct Service : service::Base
      {
         using service::Base::Base;

         service::Timeout timeout;

         CASUAL_CONST_CORRECT_SERIALIZE(
            service::Base::serialize( archive);
            CASUAL_SERIALIZE( timeout);
         )
      };

      namespace service
      {
         namespace call
         {
            //! Represent service information in a 'call context'
            struct Service : message::Service
            {
               using message::Service::Service;

               // if the requested service name differs from the 'origin', requested is set.
               std::optional< std::string> requested;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  service::Base::serialize( archive);
                  CASUAL_SERIALIZE( requested);
               )
            };
         } // call

         struct Transaction
         {
            // TODO: major-version change to short 
            enum class State : char
            {
               absent,
               active = absent,
               rollback,
               timeout,
               error,
            };

            common::transaction::ID trid;
            State state = State::active;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( trid);
               CASUAL_SERIALIZE( state);
            )

            inline friend std::ostream& operator << ( std::ostream& out, Transaction::State value)
            {
               switch( value)
               {
                  case Transaction::State::error: return out << "error";
                  case Transaction::State::active: return out << "active";
                  case Transaction::State::rollback: return out << "rollback";
                  case Transaction::State::timeout: return out << "timeout";
               }
               return out << "unknown";
            }
         };

         namespace advertise
         {
            //! Represent service information in a 'advertise context'
            using Service = message::service::Base;

         } // advertise

         using base_advertise = basic_request< message::Type::service_advertise>;
         struct Advertise : base_advertise
         {
            using base_advertise::base_advertise;

            //! the alias of the server 'instance'
            std::string alias;

            struct
            {
               std::vector< advertise::Service> add;
               std::vector< std::string> remove;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( add);
                  CASUAL_SERIALIZE( remove);
               )
            } services;


            //! @return true ii the intention is to remove all advertised services for the server
            inline bool clear() const { return services.add.empty() && services.remove.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_advertise::serialize( archive);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( services);
            )
         };

         namespace concurrent
         {
            namespace advertise
            {
               namespace service
               {
                  namespace property
                  {
                     enum class Type : short
                     {
                        configured,
                        discovered,
                     };

                     inline std::ostream& operator << ( std::ostream& out, Type value)
                     {
                        switch( value)
                        {
                           case Type::configured: return out << "configured";
                           case Type::discovered: return out << "discovered";
                        }
                        return out << "<unknown>";
                     }
                  } // property

                  struct Property : common::Compare< Property>
                  {
                     platform::size::type hops = 0;
                     property::Type type = property::Type::discovered;

                     inline auto tie() const { return std::tie( type, hops);}

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( hops);
                        CASUAL_SERIALIZE( type);
                     )
                  };


               } // service

               //! Represent service information in a 'advertise context'
               struct Service : message::Service
               {
                  Service() = default;
                  inline Service( std::string name, 
                     std::string category, 
                     common::service::transaction::Type transaction,
                     service::Property property = service::Property{})
                     : message::Service( std::move( name), std::move( category), transaction), property( property) {}

                  service::Property property;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     message::Service::serialize( archive);
                     CASUAL_SERIALIZE( property);
                  )
               };

            } // advertise

            using basic_advertise = basic_request< message::Type::service_concurrent_advertise>;
            struct Advertise : basic_advertise
            {
               using basic_advertise::basic_advertise;

               //! the alias of the server 'instance'
               std::string alias;
               platform::size::type order = 0;

               struct
               {
                  std::vector< advertise::Service> add;
                  std::vector< std::string> remove;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( add);
                     CASUAL_SERIALIZE( remove);
                  )
               } services;

               //! indicate to remove all current advertised services, and replace with content in this message
               bool reset = false;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  basic_advertise::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( reset);
               )
            };
            
         } // concurrent   


         namespace lookup
         {
            //! Represent "service-name-lookup" request.
            using base_request = basic_request< message::Type::service_name_lookup_request>; 
            struct Request : base_request
            {
               using base_request::base_request;

               enum class Context : short
               {
                  regular,
                  forward,
                  no_busy_intermediate,
                  wait,
               };

               inline friend std::ostream& operator << ( std::ostream& out, const Request::Context& value)
               {
                  switch( value)
                  {
                     case Request::Context::regular: return out << "regular";
                     case Request::Context::forward: return out << "forward";
                     case Request::Context::no_busy_intermediate: return out << "no-busy-intermediate";
                     case Request::Context::wait: return out << "wait";
                     
                  }
                  return out << "unknown";
               }

               std::string requested;
               Context context = Context::regular;
               std::optional< platform::time::point::type> deadline{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( requested);
                  CASUAL_SERIALIZE( context);
                  CASUAL_SERIALIZE( deadline);
               )
            };

            //! Represent "service-name-lookup" response.
            using base_reply = basic_reply< message::Type::service_name_lookup_reply>; 
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               enum class State : short
               {
                  absent,
                  busy,
                  idle
               };

               inline friend std::ostream& operator << ( std::ostream& out, State value)
               {
                  switch( value)
                  {
                     case Reply::State::absent: return out << "absent";
                     case Reply::State::idle: return out << "idle";
                     case Reply::State::busy: return out << "busy";
                  }
                  return out << "unknown";
               }

               call::Service service;
               //! represent how long this request was pending (busy);
               platform::time::unit pending{};
               State state = State::idle;
               
               inline bool busy() const { return state == State::busy;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( state);
               )
            };

            namespace discard
            {
               using base_request = basic_request< message::Type::service_name_lookup_discard_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::string requested;
                  //! if caller want's a reply or not, default: true
                  bool reply = true;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( requested);
                     CASUAL_SERIALIZE( reply);
                  )
               };

               using base_reply = basic_message< message::Type::service_name_lookup_discard_reply>;
               struct Reply : base_reply
               {
                  enum class State : short
                  {
                     absent,
                     discarded,
                     replied
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, State value)
                  {
                     switch( value)
                     {
                        case Reply::State::absent: return out << "absent";
                        case Reply::State::discarded: return out << "discarded";
                        case Reply::State::replied: return out << "replied";
                     }
                     return out << "unknown";
                  }

                  State state = State::absent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( state);
                  )
               };
            } // discard
         } // lookup


         namespace call
         {
            namespace request
            {
               enum class Flag : long
               {
                  no_transaction = cast::underlying( flag::xatmi::Flag::no_transaction),
                  no_reply = cast::underlying( flag::xatmi::Flag::no_reply),
                  no_time = cast::underlying( flag::xatmi::Flag::no_time),
               };
               using Flags = common::Flags< Flag>;

            } // request

            template< message::Type type>
            struct common_request : message::basic_request< type>
            {
               using base_type = message::basic_request< type>;
               using base_type::base_type;

               Service service;
               std::string parent;

               common::transaction::ID trid;
               request::Flags flags;

               common::service::header::Fields header;

               //! pending time, only to be return in the "ACK", to collect
               //! metrics
               platform::time::unit pending{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_type::serialize( archive);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( header);
                  CASUAL_SERIALIZE( pending);
               )
            };

            namespace caller
            {
               using base_request = common_request< message::Type::service_call>;

               //! Represents a service call. via tp(a)call, from the callers perspective
               struct Request : base_request
               {
                  template< typename... Args>
                  Request( common::buffer::payload::Send buffer, Args&&... args)
                     : base_request( std::forward< Args>( args)...), buffer( std::move( buffer))
                  {}
                  
                  request::Flags flags;
                  common::buffer::payload::Send buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( flags);
                     CASUAL_SERIALIZE( buffer);
                  )
               };

            } // caller

            namespace callee
            {
               using base_request = common_request< message::Type::service_call>;

               //! Represents a service call. via tp(a)call, from the callee's perspective
               struct Request : base_request
               {   
                  using base_request::base_request;

                  request::Flags flags;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( flags);
                     CASUAL_SERIALIZE( buffer);
                  )
               };
            } // callee


            //! Represent service reply.
            using base_reply = basic_message< message::Type::service_reply>;
            struct Reply : base_reply
            {
               service::Code code;
               Transaction transaction;
               common::buffer::Payload buffer;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( code);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( buffer);
               )
            };

            //! Represent the reply to the service-manager when a server is done handling
            //! a service-call and is ready for new calls
            using base_ack = basic_message< message::Type::service_acknowledge>;
            struct ACK : base_ack
            {
               event::service::Metric metric;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_ack::serialize( archive);
                  CASUAL_SERIALIZE( metric);
               )
            };
            
         } // call

      } // service

      namespace reverse
      {

         template<>
         struct type_traits< service::lookup::Request> : detail::type< service::lookup::Reply> {};

         template<>
         struct type_traits< service::lookup::discard::Request> : detail::type< service::lookup::discard::Reply> {};

         template<>
         struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

         template<>
         struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

      } // reverse

   } // common::message

} // casual


