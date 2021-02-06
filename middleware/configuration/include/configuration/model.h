//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment/variable.h"
#include "common/compare.h"
#include "common/serialize/macro.h"
#include "common/service/type.h"

#include <vector>
#include <string>

namespace casual::configuration
{
   namespace model
   {
      namespace domain
      {
         struct Environment : common::Compare< Environment>
         {
            std::vector< common::environment::Variable> variables;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( variables);
            )

            inline auto tie() const { return std::tie( variables);}
         };
            
         struct Group : common::Compare< Group>
         {
            std::string name;
            std::string note;

            std::vector< std::string> resources;
            std::vector< std::string> dependencies;

            inline friend bool operator == ( const Group& lhs, const std::string& rhs) { return lhs.name == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( dependencies);
            )

            inline auto tie() const { return std::tie( name, note, resources, dependencies);}
         };

         struct Lifetime : common::Compare< Lifetime>
         {
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( restart);
            )

            inline auto tie() const { return std::tie( restart);}
         };

         struct Executable : common::Compare< Executable>
         {
            std::string path;
            std::string alias;
            std::string note;
            std::vector< std::string> arguments;

            platform::size::type instances = 1;
            std::vector< std::string> memberships;
            Environment environment;

            Lifetime lifetime;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( arguments);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( memberships);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( lifetime);
            )

            inline auto tie() const { return std::tie( path, alias, note, arguments, instances, memberships, environment, lifetime);}
         };

         struct Server : Executable, common::Compare< Server>
         {
            std::vector< std::string> resources;
            std::vector< std::string> restrictions;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Executable::serialize( archive);   
               CASUAL_SERIALIZE( resources);
               CASUAL_SERIALIZE( restrictions);
            )

            inline auto tie() const { return std::tuple_cat( Executable::tie(), std::tie( resources, restrictions));}
         };

         struct Model : common::Compare< Model>
         {
            std::string name;
            domain::Environment environment;
            std::vector< domain::Group> groups;
            std::vector< domain::Server> servers;
            std::vector< domain::Executable> executables;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( environment);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
            )

            inline auto tie() const 
            { 
               return std::tie( name, environment, groups, servers, executables);
            }
         };

      } // domain


      namespace service
      {
         struct Timeout : common::Compare< Timeout>
         {
            using Contract = common::service::execution::timeout::contract::Type;
            
            std::optional< platform::time::unit> duration;
            Contract contract = Contract::linger;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( duration);
               CASUAL_SERIALIZE( contract);
            )

            inline auto tie() const { return std::tie( duration, contract);}
         };

         struct Service : common::Compare< Service>
         {
            std::string name;
            std::vector< std::string> routes;
            service::Timeout timeout;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( routes);
               CASUAL_SERIALIZE( timeout);
            )

            inline auto tie() const { return std::tie( name, routes, timeout);}
         };

         struct Model : common::Compare< Model>
         {  
            service::Timeout timeout;
            std::vector< service::Service> services;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( timeout);
               CASUAL_SERIALIZE( services);
            )

            inline auto tie() const 
            { 
               return std::tie( timeout, services);
            }
         };
      } // service

      namespace transaction
      {
         struct Resource : common::Compare< Resource>
         {
            std::string name;
            std::string key;
            platform::size::type instances = 0;
            std::string note;

            std::string openinfo;
            std::string closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( openinfo);
               CASUAL_SERIALIZE( closeinfo);
            )

            inline auto tie() const { return std::tie( name, key, instances, note, openinfo, closeinfo);}
         };

         struct Model : common::Compare< Model>
         {
            std::string log;
            std::vector< transaction::Resource> resources;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( log);
               CASUAL_SERIALIZE( resources);
            )

            inline auto tie() const { return std::tie( log, resources);}
         };

      } // transaction

      namespace gateway
      {
         namespace connect
         {
            enum struct Semantic : short
            {
               unknown,
               regular,
               reversed
            };
         } // connection


         namespace inbound
         {
            struct Limit : common::Compare< Limit>
            {
               platform::size::type size = 0;
               platform::size::type messages = 0;

               constexpr operator bool() const noexcept { return size > 0 || messages > 0;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )

               inline auto tie() const { return std::tie( size, messages);}
            };
            
            struct Connection : common::Compare< Connection>
            {
               std::string address;
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( address, note);}
            };

            struct Group : common::Compare< Group>
            {
               connect::Semantic connect = connect::Semantic::unknown;
               std::string alias;
               inbound::Limit limit;
               std::vector< inbound::Connection> connections;
               std::string note;

               inline bool empty() const { return connections.empty();}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( limit);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( connect, alias, limit, connections, note);}
            };

         }
         struct Inbound : common::Compare< Inbound>
         {
            std::vector< inbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         namespace outbound
         {
            struct Connection : common::Compare< Connection>
            {
               std::string address;
               std::vector< std::string> services;
               std::vector< std::string> queues;
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( address);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )

               inline explicit operator bool () const { return ! ( services.empty() && queues.empty());}

               inline auto tie() const { return std::tie( address, services, queues, note);}
            };

            struct Group : common::Compare< Group>
            {
               connect::Semantic connect = connect::Semantic::unknown;
               platform::size::type order{};
               std::string alias;
               std::vector< outbound::Connection> connections;
               std::string note;

               inline bool empty() const { return connections.empty();}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( connections);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( connect, order, alias, connections, note);}
            };
         }

         struct Outbound : common::Compare< Outbound>
         {
            std::vector< outbound::Group> groups;

            inline bool empty() const { return groups.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         struct Model : common::Compare< Model>
         {
            Inbound inbound;
            Outbound outbound;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
            )

            inline auto tie() const { return std::tie( inbound, outbound);}
         };
         
      } // gateway

      namespace queue
      {
   
         struct Queue : common::Compare< Queue>
         {
            struct Retry : common::Compare< Retry>
            {
               platform::size::type count = 0;
               platform::time::unit delay = platform::time::unit::zero();

               inline auto empty() const { return count == 0 && delay == platform::time::unit::zero();}

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )

               inline auto tie() const { return std::tie( count, delay);}
            };

            std::string name;
            Retry retry;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( name, retry, note);}
         };

         struct Group : common::Compare< Group>
         {
            std::string alias;
            std::string queuebase;
            std::string note;
            std::vector< Queue> queues;

            inline friend bool operator == ( const Group& lhs, const std::string& alias) { return lhs.alias == alias;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( queuebase);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( queues);
            )

            inline auto tie() const { return std::tie( alias, queuebase, note, queues);}
            
         };

         namespace forward
         {
            struct forward_base : common::Compare< forward_base>
            {
               std::string alias;
               std::string source;
               platform::size::type instances{};
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( source);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( alias, source, instances, note);}
            };

            struct Queue : forward_base, common::Compare< Queue>
            {
               struct Target : common::Compare< Target>
               {
                  std::string queue;
                  platform::time::unit delay{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( queue);
                     CASUAL_SERIALIZE( delay);
                  )

                  inline auto tie() const { return std::tie( queue, delay);}
               };

               Target target;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
               )

               inline auto tie() const { return std::tuple_cat( forward_base::tie(), std::tie( target));}
            };

            struct Service : forward_base, common::Compare< Service>
            {
               struct Target : common::Compare< Target>
               {
                  std::string service;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                  )
                  inline auto tie() const { return std::tie( service);}
               };

               using Reply = Queue::Target;

               Target target;
               std::optional< Reply> reply;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( reply);
               )

               inline auto tie() const { return std::tuple_cat( forward_base::tie(), std::tie( target, reply));}
            };

            struct Group : common::Compare< Group>
            {
               std::string alias;
               std::vector< forward::Service> services;
               std::vector< forward::Queue> queues;
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( note);
               )

               inline auto tie() const { return std::tie( alias, services, queues);}
            };
         } // forward

         struct Forward : common::Compare< Forward>
         {
            std::vector< forward::Group> groups;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
            )

            inline auto tie() const { return std::tie( groups);}
         };

         struct Model : common::Compare< Model>
         {
            std::vector< queue::Group> groups;
            Forward forward;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( forward);
               CASUAL_SERIALIZE( note);
            )

            inline auto tie() const { return std::tie( groups, forward, note);}
         };

      } // queue
   } // model

   struct Model : common::Compare< Model>
   {
      model::domain::Model domain;
      model::transaction::Model transaction;               
      model::service::Model service;
      model::queue::Model queue;
      model::gateway::Model gateway;

      CASUAL_CONST_CORRECT_SERIALIZE
      (
         CASUAL_SERIALIZE( domain);
         CASUAL_SERIALIZE( transaction);
         CASUAL_SERIALIZE( service);
         CASUAL_SERIALIZE( queue);
         CASUAL_SERIALIZE( gateway);
      )

      inline auto tie() const { return std::tie( domain, transaction, service, queue, gateway);}

   };

   namespace model
   {
   
      //! finalize and validates the model
      //! @throws system::invalid::Argument if something is not ok.
      void finalize( configuration::Model& model);

   } // model

} // casual::configuration