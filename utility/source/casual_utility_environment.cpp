//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_utility_environment.h"
#include "casual_exception.h"

#include <stdlib.h>


namespace casual
{
	namespace utility
	{
		namespace environment
		{
			namespace variable
			{
				bool exists( const std::string& name)
				{
					return getenv( name.c_str()) != 0;
				}

				std::string get( const std::string& name)
				{
					char* result = getenv( name.c_str());

					if( result)
					{
						return result;
					}
					else
					{
						throw exception::EnvironmentVariableNotFound( name);
					}

				}
			}
		}
	}
}
