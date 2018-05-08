//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace exception
      {

         //!
         //! throws pending exception, catches it and log based on the exception. returns the 
         //! corresponding error code.
         //!
         int handle() noexcept;

         int handle( std::ostream& out) noexcept;

      } // error
   } // common
} // casual




