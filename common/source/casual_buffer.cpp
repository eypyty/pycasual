//!
//! casual_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "casual_buffer.h"


#include <stdexcept>
#include "casual_exception.h"
#include <algorithm>

namespace casual
{
	namespace buffer
	{

		namespace local
		{
			struct FindBuffer
			{
				FindBuffer( void* toFind) : m_toFind( toFind) {}

				bool operator () ( Buffer& buffer) const
				{
					return m_toFind == buffer.raw();
				}

				void* m_toFind;
			};


		}

		Holder::Holder()
		{

		}

		Holder& Holder::instance()
		{
			static Holder singleton;
			return singleton;
		}

		Buffer& Holder::allocate(const std::string& type, const std::string& subtype, std::size_t size)
		{

			m_memoryPool.push_back( Buffer( type, subtype, size));
			return m_memoryPool.back();
		}



		Buffer& Holder::reallocate( void* memory, std::size_t size)
		{
			Buffer& buffer = *get( memory);

			buffer.reallocate( size);

			return buffer;
		}

		Holder::pool_type::iterator Holder::get( void* memory)
		{
			pool_type::iterator findIter = std::find_if(
				m_memoryPool.begin(),
				m_memoryPool.end(),
				local::FindBuffer( memory));

			if( findIter == m_memoryPool.end())
			{
				throw exception::MemoryNotFound();
			}

			return findIter;
		}

		void Holder::deallocate( void* memory)
		{
			pool_type::iterator buffer = get( memory);

			m_memoryPool.erase( buffer);

		}

	}
}



