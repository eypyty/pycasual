//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/pending/send/message.h"
#include "domain/manager/unittest/process.h"


#include "common/message/service.h"

#include "common/communication/ipc.h"

namespace casual
{

   namespace domain
   {
      namespace pending
      {
         namespace send
         {

            TEST( eventually_send, spawn_terminate)
            {
               common::unittest::Trace trace;

               EXPECT_NO_THROW( {
                  manager::unittest::Process domain;
               });
            }



            TEST( eventually_send, eventually_message__expect_sent)
            {
               common::unittest::Trace trace;

               manager::unittest::Process domain;

               common::communication::ipc::Helper ipc;

               {
                  common::message::service::lookup::Request message;
                  message.requested = "foo";
                  pending::send::message( common::process::handle(), message);
               }

               {
                  common::message::service::lookup::Request message;
                  ipc.blocking_receive( message);

                  EXPECT_TRUE( message.requested ==  "foo");
               }
            }

         } // send
      } // pending
   } // domain
} // casual
