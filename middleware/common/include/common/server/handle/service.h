//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/server/context.h"
#include "common/service/conversation/context.h"

#include "common/buffer/transport.h"
#include "common/execution.h"
#include "common/log.h"
#include "common/execute.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

#include "common/flag.h"

#include "common/message/service.h"
#include "common/message/conversation.h"

namespace casual
{
   namespace common::server::handle::service
   {
      namespace transform
      {
         message::service::call::Reply reply( const message::service::call::callee::Request& message);
         message::conversation::callee::Send reply( const message::conversation::connect::callee::Request& message);

         common::service::invoke::Parameter parameter( message::service::call::callee::Request& message);
         common::service::invoke::Parameter parameter( message::conversation::connect::callee::Request& message);

      } // transform

      namespace complement
      {
         void reply( common::service::invoke::Result&& result, message::service::call::Reply& reply);
         void reply( common::service::invoke::Result&& result, message::conversation::callee::Send& reply);

      } // complement

      template< typename P, typename C, typename M>
      void call( P& policy, C& service_context, M&& message, bool send_reply)
      {
         Trace trace{ "server::handle::service::call"};

         execution::service::name( message.service.name);
         execution::service::parent::name( message.parent);

         common::service::header::fields() = std::move( message.header);

         auto start = platform::time::clock::type::now();


         // Prepare reply
         auto reply = transform::reply( message);

         // Make sure we do some cleanup and send ACK to service-manager.
         auto execute_finalize = execute::scope( [&]()
         {
            message::service::call::ACK ack;

            ack.correlation = message.correlation;
            ack.execution = message.execution;
            ack.metric.execution = message.execution;
            ack.metric.service = message.service.name;
            ack.metric.parent = message.parent;
            ack.metric.process = common::process::handle();
            ack.metric.trid = reply.transaction.trid;

            ack.metric.start = start;
            ack.metric.end = platform::time::clock::type::now();

            // make sure service-manager "gets back" the pending metric
            ack.metric.pending = message.pending;
            
            ack.metric.code = reply.code.result;

            policy.ack( ack);
            server::context().finalize();
         });


         auto execute_reply = execute::scope( [&]()
         {
            // Send reply to caller.
            if( send_reply)
               policy.reply( message.process.ipc, reply);
         });




         // If something goes wrong, make sure to rollback before reply with error.
         // this will execute before execute_reply
         auto execute_error_reply = execute::scope( [&]()
         {
            reply.transaction = policy.transaction( false);
         });

         auto& state = server::Context::instance().state();

         // Find service
         auto found = algorithm::find( state.services, message.service.name);

         if( ! found)
         {
            code::raise::error( 
               code::xatmi::system, 
               message.service.name, " not present at server - inconsistency between service-manager and server");
         }


         auto& service = found->second;

         // Do transaction stuff...
         // - begin transaction if service has "auto-transaction"
         // - notify TM about potentially resources involved.
         // - set 'global' deadline/timeout
         policy.transaction( message.trid, service, message.service.timeout.duration, start);

         auto parameter = transform::parameter( message);
         
         // transform::parameter( message) may have reserved a descriptor that we need to
         // unreserve! 
         auto execute_unreserve_descriptor = execute::scope( [descriptor = parameter.descriptor]()
         {
            if( descriptor)
               common::service::conversation::Context::instance().descriptors().unreserve( descriptor);
         });


         // call the service
         try
         {
            complement::reply( service( std::move( parameter)), reply);
         }
         catch( common::service::invoke::Forward& forward)
         {
            policy.forward( std::move( forward), message);
            
            policy.transaction( true);

            execute_reply.release();
            execute_error_reply.release();

            // reply.code.result is used to set outcomme in the 'ack'. We might want a _forward_ code?
            reply.code.result = code::xatmi::ok;

            return;
         }

         // TODO: What are the semantics of 'order' of failure?
         //       If TM is down, should we send reply to caller?
         //       If broker is down, should we send reply to caller?


         // Do transaction stuff...
         // - commit/rollback transaction if service has "auto-transaction"
         auto execute_transaction = execute::scope( [&]()
         {
            reply.transaction = policy.transaction(
                  reply.transaction.state == message::service::Transaction::State::active);
         });

         // Nothing did go wrong
         execute_error_reply.release();

         execute_transaction();
         execute_reply();
         execute_unreserve_descriptor();
      }


   } // common::server::handle::service

} // casual


