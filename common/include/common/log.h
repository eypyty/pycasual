//!
//! casual_logger.h
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_LOGGER_H_
#define CASUAL_LOGGER_H_



#include "common/platform.h"


#include <string>
#include <ostream>
#include <mutex>

namespace casual
{
   namespace common
   {
      namespace log
      {

         namespace category
         {
            enum class Type
            {
               //
               // casual internal logging
               casual_debug,
               casual_trace,
               casual_transaction,
               casual_ipc,
               casual_queue,

               //
               // Public logging
               debug = 10,
               trace,
               parameter,
               information,
               warning,
               error,
            };

            const char* name( Type type);

         } // category

         namespace thread
         {
            class Safe
            {
            public:
               Safe( std::ostream& stream) : m_stream( stream), m_lock( m_mutext) {}
               Safe( Safe&&) = default;


               template< typename T>
               Safe& operator << ( T&& value)
               {
                  m_stream << std::forward< T>( value);
                  return *this;
               }


               typedef std::ostream& (&omanip_t)( std::ostream&);

               //!
               //! Overload for manip-functions...
               //! @note Why does not the T&& take these?
               //!
               Safe& operator << ( omanip_t value)
               {
                  m_stream << value;
                  return *this;
               }

            private:
               std::ostream& m_stream;
               std::unique_lock< std::mutex> m_lock;

               static std::mutex m_mutext;
            };


         } // thread


         namespace internal
         {

            class Stream : public std::ostream
            {
            public:

               using std::ostream::ostream;


               template< typename T>
               thread::Safe operator << ( T&& value)
               {
                  thread::Safe proxy{ *this};
                  proxy << std::forward< T>( value);
                  return proxy;
               }
            };
         } // internal



         //!
         //! Log with category 'debug'
         //!
         extern internal::Stream debug;

         //!
         //! Log with category 'trace'
         //!
         extern internal::Stream trace;


         //!
         //! Log with category 'parameter'
         //!
         extern internal::Stream parameter;

         //!
         //! Log with category 'information'
         //!
         extern internal::Stream information;

         //!
         //! Log with category 'warning'
         //!
         //! @note should be used very sparsely. Either it is an error or it's not...
         //!
         extern internal::Stream warning;

         //!
         //! Log with category 'error'
         //!
         //! @note always active
         //!
         extern internal::Stream error;

         //!
         //! @return true if the log-category is active.
         //!
         bool active( category::Type category);

         void activate( category::Type category);

         void deactivate( category::Type category);


         //std::ostream& stream( category::Type category);


         void write( category::Type category, const char* message);

         void write( category::Type category, const std::string& message);


      } // log

   } // common

} // casual

#endif /* CASUAL_LOGGER_H_ */
