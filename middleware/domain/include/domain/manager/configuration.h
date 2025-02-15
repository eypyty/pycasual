//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

#include "configuration/model.h"

namespace casual
{
   namespace domain::manager::configuration
   {
      
      //! @returns the total state of all managers.
      casual::configuration::Model get( const State& state);

      //! @pre state.configuration.model is set to the current aggregated configuration model.
      //! @return id's of tasks that tries to get to the wanted state
      std::vector< common::strong::correlation::id> post( State& state, casual::configuration::Model wanted);


   } // domain::manager::configuration
} // casual


