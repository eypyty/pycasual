//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/common.h"

#include "common/communication/ipc.h" 
#include "common/message/pending.h"

#include "common/code/casual.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct State;

         namespace ipc
         {

            common::communication::ipc::inbound::Device& device();

            namespace pending
            {
               void send( const State& state, common::message::pending::Message&& pending);
               
            } // pending

            void send( const State& state, common::message::pending::Message&& pending);

            template< typename M>
            void send( const State& state, const common::process::Handle& process, M&& message)
            {
               try
               {
                  if( ! common::communication::device::non::blocking::send( process.ipc, message))
                     ipc::pending::send( state, common::message::pending::Message{ std::forward< M>( message), process});
               }
               catch( ...)
               {
                  if( common::exception::capture().code() != common::code::casual::communication_unavailable)
                     throw;

                  common::log::line( domain::log, common::code::casual::communication_unavailable, " failed to send message - type: ", common::message::type( message), " to: ", process, " - action: ignore");
                  common::log::line( domain::verbose::log, "message: ", message);
               }
            }
            
            //! push the message to our own ipc queue
            template< typename M>
            void push( M&& message)
            {
               ipc::device().push( std::forward< M>( message));
            }


         } // ipc
      } // manager
   } // domain
} // casual