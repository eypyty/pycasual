//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_


#include <thread>

#include "common/mockup/log.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         //!
         //! as std::thread, but sends signal terminate on destruction
         //! and joins the thread
         //!
         struct Thread
         {
            Thread() noexcept;

            template< typename... Args>
            Thread( Args&&... args)
               : m_thread{ std::forward< Args>( args)...}
            {
               log << "mockup::Thread ctor - thread: " << m_thread.get_id() << '\n';
            }

            Thread( Thread&&) noexcept;
            Thread& operator = ( Thread&&) noexcept;

            ~Thread();

         private:

            std::thread m_thread;
         };
      } // mockup
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_
