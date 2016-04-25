//!
//! json.cpp
//!
//! Created on: Jan 18, 2015
//!     Author: Lazan
//!

#include "sf/archive/json.h"


#include "common/transcode.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <iterator>
#include <istream>
#include <functional>


#include <iostream> // debug

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace json
         {

            Load::Load() = default;
            Load::~Load() = default;

            const rapidjson::Document& Load::serialize( std::istream& stream)
            {
               //
               // TODO: Use ParseStream with newer version of rapidjson
               //
               const std::string json{
                  std::istream_iterator< char>( stream),
                  std::istream_iterator< char>()};

               return serialize( json);
            }

            const rapidjson::Document& Load::serialize( const std::string& json)
            {
               return serialize( json.c_str());
            }

            const rapidjson::Document& Load::serialize( const char* const json, const std::size_t size)
            {
               // To ensure null-terminated string
               return serialize( std::string( json, size).c_str());
            }

            const rapidjson::Document& Load::serialize( const char* const json)
            {
               m_document.Parse( json);

               if( m_document.HasParseError())
               {
                  throw exception::archive::invalid::Document{ rapidjson::GetParseError_En( m_document.GetParseError())};
               }

               return m_document;
            }

            const rapidjson::Document& Load::source() const
            {
               return m_document;
            }


            namespace reader
            {

               Implementation::Implementation( const rapidjson::Value& document) : m_stack{ &document} {}
               Implementation::~Implementation() = default;

               std::tuple< std::size_t, bool> Implementation::container_start( std::size_t size, const char* const name)
               {
                  if( ! start( name))
                  {
                     return std::make_tuple( 0, false);
                  }

                  const auto& node = *m_stack.back();

                  //
                  // This check is to avoid terminate (via assert)
                  //
                  if( ! node.IsArray())
                  {
                     throw exception::archive::invalid::Node{ "expected array"};
                  }

                  //
                  // Stack 'em backwards
                  //

                  size = node.Size();

                  for( auto index = size; index > 0; --index)
                  {
                     m_stack.push_back( &node[ index - 1]);
                  }

                  return std::make_tuple( size, true);

               }

               void Implementation::container_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::serialtype_start( const char* const name)
               {
                  if( ! start( name))
                  {
                     return false;
                  }

                  //
                  // This check is to avoid terminate (via assert)
                  //
                  if( ! m_stack.back()->IsObject())
                  {
                     throw exception::archive::invalid::Node{ "expected object"};
                  }

                  return true;

               }

               void Implementation::serialtype_end( const char* const name)
               {
                  end( name);
               }

               bool Implementation::start( const char* const name)
               {
                  if( name)
                  {
                     //
                     // TODO: Remove this check whenever archive is fixed
                     //
                     if( m_stack.back())
                     {
                        const auto result = m_stack.back()->FindMember( name);

                        if( result != m_stack.back()->MemberEnd())
                        {
                           m_stack.push_back( &result->value);
                        }
                        else
                        {
                           m_stack.push_back( nullptr);
                        }
                     }
                     else
                     {
                        m_stack.push_back( nullptr);
                     }
                  }

                  return m_stack.back() != nullptr;
               }

               void Implementation::end( const char* const name)
               {
                  m_stack.pop_back();
               }


               namespace
               {
                  namespace local
                  {
                     //
                     // This is a help to check some to avoid terminate (via assert)
                     //
                     template<typename C, typename F>
                     auto read( const rapidjson::Value* const value, C&& checker, F&& fetcher) -> decltype( std::bind( fetcher, value)())
                     {
                        if( std::bind( checker, value)())
                        {
                           return std::bind( fetcher, value)();
                        }

                        throw exception::archive::invalid::Node{ "unexpected type"};
                     }
                  } // local
               } // <unnamed>


               void Implementation::read( bool& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsBool, &rapidjson::Value::GetBool); }
               void Implementation::read( short& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsInt, &rapidjson::Value::GetInt); }
               void Implementation::read( long& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
               void Implementation::read( long long& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
               void Implementation::read( float& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
               void Implementation::read( double& value) const
               { value = local::read( m_stack.back(), &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
               void Implementation::read( char& value) const
               { value = *common::transcode::utf8::decode( local::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)).c_str(); }
               void Implementation::read( std::string& value) const
               { value = common::transcode::utf8::decode( local::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)); }
               void Implementation::read( platform::binary_type& value) const
               { value = common::transcode::base64::decode( local::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)); }

            } // reader


            Save::Save()
            {
               m_document.SetObject();
            }

            Save::~Save() = default;

            void Save::serialize( std::ostream& stream) const
            {
               rapidjson::StringBuffer buffer;
               rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer);
               if( m_document.Accept( writer))
               {
                  stream << buffer.GetString();
               }
               else
               {
                  //
                  // TODO: Better
                  //

                  throw exception::archive::invalid::Document{ "Failed to write document"};
               }
            }

            void Save::serialize( std::string& json) const
            {
               rapidjson::StringBuffer buffer;
               rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer);

               if( m_document.Accept( writer))
               {
                  json = buffer.GetString();
               }
               else
               {
                  //
                  // TODO: Better
                  //

                  throw exception::archive::invalid::Document{ "Failed to write document"};
               }

            }

            rapidjson::Document& Save::target()
            {
               return m_document;
            }


            namespace writer
            {

               Implementation::Implementation( rapidjson::Document& document)
               : m_allocator( document.GetAllocator()), m_stack{ &document}
               {}


               Implementation::Implementation( rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator)
               : m_allocator( allocator), m_stack{ &object}
               {}

               Implementation::~Implementation() = default;

               std::size_t Implementation::container_start( std::size_t size, const char* const name)
               {
                  start( name);

                  m_stack.back()->SetArray();

                  return size;
               }

               void Implementation::container_end( const char* const name)
               {
                  end( name);
               }


               void Implementation::serialtype_start( const char* const name)
               {
                  start( name);

                  m_stack.back()->SetObject();
               }
               void Implementation::serialtype_end( const char* const name)
               {
                  end( name);
               }


               void Implementation::start( const char* const name)
               {
                  //
                  // Both AddMember and PushBack returns the parent (*this)
                  // instead of a reference to the added value (despite what
                  // the documentation says) and thus we need to do some
                  // cumbersome iterator stuff to get a reference/pointer to it
                  //


                  auto& parent = *m_stack.back();

                  if( name)
                  {
                     //parent.AddMember( name, rapidjson::Value(), m_allocator);
                     parent.AddMember( rapidjson::Value( name, m_allocator), rapidjson::Value(), m_allocator);

                     m_stack.push_back( &(*(parent.MemberEnd() - 1)).value);
                  }
                  else
                  {
                     //
                     // We are in a container
                     //

                     parent.PushBack( rapidjson::Value(), m_allocator);

                     m_stack.push_back( &(*(parent.End() - 1)));

                  }


               }

               void Implementation::end( const char* name)
               {
                  m_stack.pop_back();
               }


               void Implementation::write( const bool value)
               { m_stack.back()->SetBool( value); }
               void Implementation::write( const char value)
               { write( std::string{ value}); }
               void Implementation::write( const short value)
               { m_stack.back()->SetInt( value); }
               void Implementation::write( const long value)
               { m_stack.back()->SetInt64( value); }
               void Implementation::write( const long long value)
               { m_stack.back()->SetInt64( value); }
               void Implementation::write( const float value)
               { m_stack.back()->SetDouble( value); }
               void Implementation::write( const double value)
               { m_stack.back()->SetDouble( value); }
               void Implementation::write( const std::string& value)
               { m_stack.back()->SetString( common::transcode::utf8::encode( value), m_allocator); }
               void Implementation::write( const platform::binary_type& value)
               { m_stack.back()->SetString( common::transcode::base64::encode( value), m_allocator); }

            } // writer

         } // json
      } // archive

   } // sf



} // casual
