//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/archive/archive.h"

#include "serviceframework/platform.h"
#include "serviceframework/pimpl.h"

#include <string>
#include <iosfwd>


namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace reader
         {
            struct File 
            {
               template< typename C>
               File( const std::string& name, C&& creator);
               ~File();

               File( File&&);
               File& operator = ( File&&);

               template< typename T>  
               archive::Reader& operator >> ( T&& value)
               {
                  return m_reader >> std::forward< T>( value); 
               }

               inline void validate() { m_reader.validate();}

            private:
               struct Implementation;
               move::Pimpl< Implementation> m_implementation; 
               archive::Reader m_reader;
            };
            namespace from
            {
               File file( const std::string& name);

               //! @{
               archive::Reader data();
               archive::Reader data( std::istream& stream);
               //! @}

               //! @{
               archive::Reader name( std::string name);
               archive::Reader name( std::istream& stream, std::string name);
               //! @}

               archive::Reader buffer( const platform::binary::type& data, std::string name);
            } // from

            namespace consumed
            {
               namespace from
               {
                  File file( const std::string& name);

                  //! @{
                  archive::Reader data();
                  archive::Reader data( std::istream& stream);
                  //! @}

                  //! @{
                  archive::Reader name( std::string name);
                  archive::Reader name( std::istream& stream, std::string name);
                  //! @}

                  archive::Reader buffer( const platform::binary::type& data, std::string name);
               } // from
            } // consumed
         } // reader




         namespace writer
         {
            namespace from
            {
               struct File 
               {
                  File( const std::string& name);
                  ~File();

                  File( File&&);
                  File& operator = ( File&&);

                  template< typename T>  
                  archive::Writer& operator << ( T&& value)
                  {
                     return m_writer << std::forward< T>( value); 
                  }

               private:
                  struct Implementation;
                  move::Pimpl< Implementation> m_implementation; 
                  archive::Writer m_writer;
               };

               File file( const std::string& name);

               //! @{
               archive::Writer name( std::string name);
               archive::Writer name( std::ostream& stream, std::string type);
               //! @}

               archive::Writer buffer( platform::binary::type& data, std::string type);
            } // from
         } // writer
      } // archive
   } // serviceframework
} // casual


