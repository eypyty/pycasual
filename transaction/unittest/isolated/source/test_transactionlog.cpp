//!
//! test_transactionlog.cpp
//!
//! Created on: Nov 3, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/log.h"

#include "common/file.h"

namespace casual
{
   namespace transaction
   {
      namespace local
      {
         common::message::transaction::begin::Request beginReqest()
         {
            common::message::transaction::begin::Request result;

            result.trid = common::transaction::ID::create();
            result.process.queue = 100;
            result.start = common::platform::clock_type::now();


            return result;
         }

         std::string transactionLogPath()
         {
            return ":memory:";
         }

      } // local

      TEST( casual_transaction_log, one_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::beginReqest();

         log.begin( begin);


         auto row = log.select( begin.trid);

         ASSERT_TRUE( row.size() == 1);
         EXPECT_TRUE( row.front().trid == begin.trid) << "row.front().trid: " << row.front().trid << std::endl;

      }

      TEST( casual_transaction_log, two_begin__expect_store)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto first = local::beginReqest();
         log.begin( first);

         auto second = local::beginReqest();
         log.begin( second);


         auto rows = log.select();

         ASSERT_TRUE( rows.size() == 2);
         EXPECT_TRUE( rows.at( 0).trid == first.trid);
         EXPECT_TRUE( rows.at( 1).trid == second.trid);

         EXPECT_TRUE( rows.at( 0).trid != rows.at( 1).trid);

      }

      TEST( casual_transaction_log, first_begin_then_prepare__expect_state_change)
      {
         auto path = local::transactionLogPath();
         Log log( path);

         auto begin = local::beginReqest();
         log.begin( begin);

         log.prepare( begin.trid);


         auto rows = log.select( begin.trid);

         ASSERT_TRUE( rows.size() == 1);
         EXPECT_TRUE( rows.at( 0).trid == begin.trid);
         EXPECT_TRUE( rows.at( 0).state == Log::State::cPrepared);
      }

      TEST( casual_transaction_log, one_begin__0us_timoeut__expect_NO_timeout)
      {
         Log log( ":memory:");

         auto begin = local::beginReqest();
         begin.timeout = std::chrono::microseconds{ 0};

         log.begin( begin);

         EXPECT_TRUE( log.timeout() == std::chrono::microseconds::min());

         auto trans = log.passed( common::platform::clock_type::now() - begin.timeout - std::chrono::seconds{ 1});

         EXPECT_TRUE( trans.empty()) << "size: " << trans.size() << std::endl;
      }

      TEST( casual_transaction_log, one_begin__10us_timoeut__expect_timeout)
      {
         Log log( ":memory:");

         auto begin = local::beginReqest();
         begin.timeout = std::chrono::microseconds{ 10};

         log.begin( begin);

         auto timeout = log.timeout();
         EXPECT_TRUE( timeout != std::chrono::microseconds::min());

         auto trans = log.passed( common::platform::clock_type::now() - timeout);

         ASSERT_TRUE( trans.size() == 1) << "trans.size(): " << trans.size() << std::endl;
         EXPECT_TRUE( trans.front() == begin.trid) << "trans.front(): " << trans.front() << std::endl;

      }


   } // transaction
} // casual
