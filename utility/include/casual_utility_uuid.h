//!
//! casual_utility_uuid.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_UUID_H_
#define CASUAL_UTILITY_UUID_H_

#include <uuid/uuid.h>

#include <string>

namespace casual
{

	namespace utility
	{

		struct uuid
		{
			uuid();

			std::string getString() const;
			const uuid_t& get() const;

			bool operator < ( const uuid& rhs) const;
			bool operator == ( const uuid& rhs) const;

		private:
			uuid_t m_uuid;

		};

	}
}



#endif /* CASUAL_UTILITY_UUID_H_ */
