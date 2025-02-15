//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/buffer/pool.h"


namespace casual
{

   namespace common
   {
      namespace buffer
      {

         struct x_octet : public pool::default_pool
         {
            static const std::vector< std::string>& types()
            {
               static const std::vector< std::string> result{
                     type::x_octet(), type::binary(), type::json(), type::yaml(), type::xml(), type::ini()
                  };
               return result;
            }
         };

         //
         // Register the pool to the pool-holder
         //
         template class pool::Registration< x_octet>;

      } // buffer
   } // common

} // casual
