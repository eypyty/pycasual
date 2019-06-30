//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "gateway/manager/listen.h"

#include "common/process.h"
#include "common/domain.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"

#include "common/message/coordinate.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace state
         {
            using size_type = common::platform::size::type;
            
            struct base_connection
            {

               enum class Runlevel
               {
                  absent,
                  connecting,
                  online,
                  offline,
                  error
               };
               friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);

               struct Address
               {
                  std::string local;
                  std::string peer;
               };


               bool running() const;

               common::process::Handle process;
               common::domain::Identity remote;
               Address address;
               Runlevel runlevel = Runlevel::absent;

               friend bool operator == ( const base_connection& lhs, common::strong::process::id rhs);
               inline friend bool operator == ( common::strong::process::id lhs, const base_connection& rhs)
               {
                  return rhs == lhs;
               }

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               { 
                  CASUAL_NAMED_VALUE( process);
                  CASUAL_NAMED_VALUE( remote);
                  CASUAL_NAMED_VALUE( address);
                  CASUAL_NAMED_VALUE( runlevel);
               })
            };

            namespace inbound
            {
               struct Connection : base_connection
               {
               };


            } // inbound

            namespace outbound
            {
               struct Connection : base_connection
               {
                  //! configured services
                  std::vector< std::string> services;

                  //! configured queues
                  std::vector< std::string> queues;

                  size_type order = 0;
                  bool restart = false;

                  void reset();

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  { 
                     base_connection::serialize( archive);
                     CASUAL_NAMED_VALUE( services);
                     CASUAL_NAMED_VALUE( queues);
                     CASUAL_NAMED_VALUE( order);
                     CASUAL_NAMED_VALUE( restart);
                  })
               };

            } // outbound

            struct Connections
            {
               std::vector< outbound::Connection> outbound;
               std::vector< inbound::Connection> inbound;
            };

            namespace coordinate
            {
               namespace outbound
               {

                  struct Policy
                  {
                     using message_type = common::message::gateway::domain::discover::accumulated::Reply;

                     void accumulate( message_type& message, common::message::gateway::domain::discover::Reply& reply);
                     void send( common::strong::ipc::id queue, message_type& message);
                  };

                  using Discover = common::message::Coordinate< Policy>;

               } // outbound
            } // coordinate
         } // state

         struct State
         {
            enum class Runlevel
            {
               startup,
               online,
               shutdown
            };
            friend std::ostream& operator << ( std::ostream& out, const Runlevel& value);

            //! @return true if we have any running connections or listeners
            bool running() const;

            void add( listen::Entry entry);
            void remove( common::strong::file::descriptor::id listener);
            listen::Connection accept( common::strong::file::descriptor::id listener);

            inline const std::vector< listen::Entry>& listeners() const { return m_listeners;}
            inline const std::vector< common::strong::file::descriptor::id>& descriptors() const { return m_descriptors;}

            common::communication::select::Directive directive;
            state::Connections connections;
            
            struct Discover
            {
               state::coordinate::outbound::Discover outbound;
               void remove( common::strong::process::id pid);

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               { 
                  CASUAL_NAMED_VALUE( outbound);
               })

            } discover;

            Runlevel runlevel = Runlevel::startup;

            CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
            { 
               CASUAL_NAMED_VALUE( directive);
               CASUAL_NAMED_VALUE( connections);
               CASUAL_NAMED_VALUE( discover);
               CASUAL_NAMED_VALUE_NAME( m_listeners, "listeners");
               CASUAL_NAMED_VALUE_NAME( m_descriptors, "descriptors");
            })

         private:
            std::vector< listen::Entry> m_listeners;
            std::vector< common::strong::file::descriptor::id> m_descriptors;
         };

      } // manager
   } // gateway
} // casual


