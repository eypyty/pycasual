//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/unittest/configuration.h"
#include "domain/common.h"
#include "domain/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"

#include "common/event/listen.h"

#include "common/message/handle.h"
#include "common/message/dispatch.h"


namespace casual
{
   using namespace common;

   namespace domain::manager::unittest::configuration
   {

      casual::configuration::user::Model get()
      {
         Trace trace{ "domain::manager::unittest::configuration::get"};

         serviceframework::service::protocol::binary::Call call;
         auto reply = call( admin::service::name::configuration::get);
         return reply.extract< casual::configuration::user::Model>();
      }

      casual::configuration::user::Model post( casual::configuration::user::Model wanted)
      {
         Trace trace{ "domain::manager::unittest::configuration::post"};

         std::vector< common::strong::correlation::id> tasks;

         auto condition = common::event::condition::compose(
            common::event::condition::prelude( [&]()
            {
               serviceframework::service::protocol::binary::Call call;
               tasks = call( admin::service::name::configuration::post, wanted).extract< std::vector< common::strong::correlation::id>>();
            }),
            common::event::condition::done( [&tasks](){ return tasks.empty();})
         );

         // listen for events
         common::event::listen( 
            condition,
            message::dispatch::handler( communication::ipc::inbound::device(),
               [&tasks]( message::event::Task& event)
               {
                  log::line( verbose::log, "event: ", event);

                  if( event.done())
                     algorithm::container::trim( tasks, algorithm::remove( tasks, event.correlation));
               })
            );

         return configuration::get();

      }

   } // domain::manager::unittest::configuration
} // casual