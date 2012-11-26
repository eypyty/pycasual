//!
//! casual_isolatedunittest_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/buffer_context.h"


namespace casual
{
   namespace common
   {

      namespace buffer
      {


         TEST( casual_common, buffer_allocate)
         {
            const char* buffer = Context::instance().allocate( "STRING", "", 2048);

            EXPECT_TRUE( buffer != nullptr);

            Context::instance().deallocate( buffer);

         }


         TEST( casual_common, buffer_reallocate)
         {
            const char* buffer = Context::instance().allocate( "STRING", "", 2048);

            EXPECT_TRUE( buffer != nullptr);

            const char* buffer2 = Context::instance().reallocate( buffer, 4096);

            EXPECT_TRUE( buffer2 != buffer);


            Context::instance().deallocate( buffer2);

         }
      }
	}
}



