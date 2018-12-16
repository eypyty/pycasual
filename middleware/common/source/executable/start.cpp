//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/executable/start.h"

#include "common/log.h"
#include "common/exception/handle.h"
#include "common/exception/system.h"

#include "common/transaction/context.h"

namespace casual
{
   namespace common
   {
      namespace executable
      {
         inline namespace v1
         {
            int start( std::vector< argument::transaction::Resource> resources, std::function< int()> user_main)
            {
               Trace trace{ "common::executable::start"};

               // validate that all resources are named
               if( algorithm::any_of( resources, [](auto& r){ return r.name.empty();}))
                  throw exception::system::invalid::Argument{ "casual resource executable has to be built with named resources"};

               // configure resources
               transaction::context().configure( std::move( resources), {});

               return user_main();
            }
         } // v1
      } // executable
   } // common
} // casual