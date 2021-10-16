//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/environment.h"

#include "common/exception/handle.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/code/convert.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>

#include <utility>


std::ostream& operator << ( std::ostream& out, const ::iovec& value)
{
   return out << "{ iov_len: " << value.iov_len << '}';
}

namespace casual
{
   namespace common::communication::ipc
   {
      namespace message
      {
         namespace transport
         {
            std::ostream& operator << ( std::ostream& out, const Header& value)
            {
               out << "{ type: " << value.type << ", correlation: ";
               transcode::hex::encode( out, value.correlation);
               return stream::write( out, ", offset: ", value.offset, ", count: ", value.count, ", size: ", value.size, '}');
            }
            
            static_assert( max::size::message() == platform::ipc::transport::size, "ipc message is too big'");
         } // transport
         

         std::ostream& operator << ( std::ostream& out, const Transport& value)
         {
            return stream::write( out, 
               "{ header: " , value.message.header
               , ", payload.size: " , value.payload_size()
               , ", header-size: " , transport::header::size()
               , ", transport-size: " ,  value.size()
               , ", max-size: " , transport::max::size::message() 
               , '}');
         }
      } // message

      namespace detail
      {

         Descriptor::Descriptor( Descriptor&& other) noexcept 
            : m_descriptor( std::exchange( other.m_descriptor, {}))
         {}

         Descriptor& Descriptor::operator = ( Descriptor&& other) noexcept
         {
            std::swap( m_descriptor, other.m_descriptor);
            return *this;
         }

         Descriptor::~Descriptor()
         {
            if( *this)
               exception::guard( [&]()
               {
                  log::line( communication::verbose::log, "remove descriptor: ", *this);

                  auto scope = signal::thread::scope::Block{};

                  posix::result( ::close( m_descriptor.value()), "close descriptor: ", *this);

               });
         }         
      } // detail


      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto path( const strong::ipc::id& id)
               {
                  return environment::directory::ipc() / uuid::string( id.value());
               }
            } // ipc

            namespace check
            {            
               bool error( std::errc code)
               {
                  log::line( verbose::log, "check error - code: ", code);

                  switch( code)
                  {
                     case std::errc::resource_unavailable_try_again:
                        return false;

#if EAGAIN != EWOULDBLOCK
                     case std::errc::operation_would_block:
                        return false;
#endif 
                  
                     case std::errc::invalid_argument:
                        code::raise::error( code::casual::invalid_argument);

                     case std::errc::broken_pipe:
                     case std::errc::bad_file_descriptor:
                     case std::errc::no_such_file_or_directory:
                     case std::errc::no_such_device:
                     case std::errc::no_such_device_or_address:
                        code::raise::error( code::casual::communication_unavailable);

                     default:
                        // will allways throw
                        code::raise::error( code::casual::internal_unexpected_value, "ipc - errno: ", code);
                  }
               }

               bool error()
               {
                  return check::error( code::system::last::error());
               } 
            } // check

            namespace file::descriptor::option
            {
               enum struct Flag : int
               {
                  no_block = O_NONBLOCK,
               };

               namespace detail
               {
                  template< typename Op>
                  void modify( strong::file::descriptor::id descriptor, Flag flag, Op predicate)
                  {
                     auto flags = ::fcntl( descriptor.value(), F_GETFL);
                     if( ::fcntl( descriptor.value(), F_SETFL, predicate( flags, cast::underlying( flag))) == -1)
                        check::error();
                  }
               } // detail

               void set( strong::file::descriptor::id descriptor, Flag flag)
               {
                  detail::modify( descriptor, flag, []( auto flags, auto flag){ return flags | flag;});
               }

               void unset( strong::file::descriptor::id descriptor, Flag flag)
               {
                  detail::modify( descriptor, flag, []( auto flags, auto flag){ return flags & ~flag;});
               }
            } // file::descriptor::option
            
            namespace transport
            {
               struct Message
               {
                  Message( const message::Complete& complete)
                  {
                     header.type = complete.type();
                     header.size = complete.size();
                     complete.correlation().value().copy( header.correlation);

                     io[ 0].iov_base = &header;
                     io[ 0].iov_len = message::transport::header::size();
                  }

                  //! sets the next chunk
                  //! @returns the new offset 
                  template< typename T>
                  platform::size::type next( T& payload, platform::size::type offset) noexcept
                  {
                     io[ 1].iov_base = const_cast< char*>( payload.data()) + offset;
                     header.offset = offset;

                     auto size = range::size( payload) - offset;

                     if( size <= message::transport::max::size::payload())
                     {   
                        io[ 1].iov_len = size;
                        header.count = size;
                        return offset + size;
                     }

                     io[ 1].iov_len = message::transport::max::size::payload();
                     header.count = message::transport::max::size::payload();
                     return offset + message::transport::max::size::payload();
                  }

                  inline platform::size::type size() const noexcept { return io[ 0].iov_len + io[ 1].iov_len;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( header);
                     CASUAL_SERIALIZE( io);
                  )

                  std::array< ::iovec, 2> io{};   
                  message::transport::Header header{};
               };

               bool send( const Handle& handle, const Message& transport)
               {
                  Trace trace{ "common::communication::ipc::local::transport::send"};

                  auto result = ::writev( handle.descriptor().value(), transport.io.data(), transport.io.size());
                  
                  if( result == 0)
                     return false;

                  if( result == -1)
                     return check::error();
                  
                  log::line( verbose::log, "ipc ---> send - handle: ", handle, ", transport: ", transport);

                  assert( result == transport.size());

                  return true;
               }


               bool receive( const Handle& handle, message::Transport& transport)
               {
                  Trace trace{ "common::communication::ipc::local::transport::receive"};

                  auto descriptor = handle.descriptor();

                  auto read = []( auto descriptor, auto first, auto last)
                  {
                     auto result = ::read( descriptor.value(), first, last - first);
                     
                     if( result == -1 && ! check::error())
                        return first;

                     return first + result;
                  };

                  // read the header
                  {
                     auto first = transport.header_data();
                     const auto last = first + message::transport::header::size();
                     first = read( descriptor, first, last);

                     if( first == transport.header_data())
                        return false;

                     while( first != last)
                        first = read( descriptor, first, last);
                  }

                  // read the payload
                  {
                     auto first = transport.payload_data();
                     const auto last = first + transport.payload_size();
                     
                     while( first != last)
                        first = read( descriptor, first, last);
                  }

                  log::line( verbose::log, "ipc <--- receive - handle: ", handle, ", transport: ", transport);

                  return true;
               }


            } // transport


            bool send( const Handle& handle, const message::Complete& complete)
            {
               local::transport::Message transport{ complete};
               platform::size::type offset{};

               do
               {
                  offset = transport.next( complete.payload, offset);
                  if( ! transport::send( handle, transport))
                     return false;
               } 
               while( offset != complete.size());

               return true;
            }



            policy::cache_range_type receive( const Handle& handle, policy::cache_type& cache)
            {
               Trace trace{ "common::communication::ipc::local::receive"};
               message::Transport transport;

               if( ! transport::receive( handle, transport))
                  return {};

               auto is_complete = [&correlation = transport.correlation()]( auto& complete)
               {
                  return ! complete.complete() && correlation == complete.correlation();
               };

               if( auto found = algorithm::find_if( cache, is_complete))
               {
                  found->add( transport);
                  return found;
               }

               cache.emplace_back( transport);
               return range::make( std::prev( std::end( cache)), 1);
            }

            
         } // <unnamed>
      } // local


      namespace policy
      {
         namespace blocking
         {
            cache_range_type receive( const Handle& handle, cache_type& cache)
            {
               Trace trace{ "common::communication::ipc::policy::blocking::receive"};
               local::file::descriptor::option::unset( handle.descriptor(), local::file::descriptor::option::Flag::no_block);

               // make sure there something to read and manage signal race conditions
               select::block::read( handle.descriptor());

               return local::receive( handle, cache);
            }

            strong::correlation::id send( const Handle& handle, const message::Complete& complete)
            {
               Trace trace{ "common::communication::ipc::policy::blocking::send"};
               local::file::descriptor::option::unset( handle.descriptor(), local::file::descriptor::option::Flag::no_block);

               // make sure there is room to write and manage signal race conditions
               select::block::write( handle.descriptor());

               if( local::send( handle, complete))
                  return complete.correlation();
               
               return {};
            }

         } // blocking

         namespace non
         {
            namespace blocking
            {
               cache_range_type receive( const Handle& handle, cache_type& cache)
               {
                  Trace trace{ "common::communication::ipc::policy::non::blocking::receive"};
                  local::file::descriptor::option::set( handle.descriptor(), local::file::descriptor::option::Flag::no_block);

                  return local::receive( handle, cache);
               }

               strong::correlation::id send( const Handle& destination, const message::Complete& complete)
               {
                  Trace trace{ "common::communication::ipc::policy::non::blocking::send"};
                  local::file::descriptor::option::set( destination.descriptor(), local::file::descriptor::option::Flag::no_block);

                  if( local::send( destination, complete))
                     return complete.correlation();
                  
                  return {};
               }
            } // blocking
         } // non


      } // policy

      namespace inbound
      {
         namespace local
         {
            namespace
            {
               namespace create
               {
                  auto fifo()
                  {
                     strong::ipc::id ipc{ uuid::make()};

                     auto file = ipc::local::ipc::path( ipc).string();

                     posix::result( ::mkfifo( file.data(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP), "create::fifo - file: ", file);

                     log::line( verbose::log, "FIFO created - ", file);

                     auto descriptor = posix::result( ::open( file.data(), O_RDONLY | O_NONBLOCK | O_CLOEXEC), "::open fifo: ", file);

                     return Handle{ strong::file::descriptor::id{ descriptor}, ipc};
                  }

                  namespace dummy
                  {
                     auto writer( strong::ipc::id ipc)
                     {
                        auto file = ipc::local::ipc::path( ipc).string();
                        return strong::file::descriptor::id{ posix::result( ::open( file.data(), O_WRONLY | O_CLOEXEC), "::open dummy-writer - file: ", file)};
                     }
                  } // dummy
               } // create
            } // <unnamed>
         } // local

         Connector::Connector()
            : m_handle{ local::create::fifo()}, m_writer{ local::create::dummy::writer( m_handle.ipc())}
         {}

         Connector::~Connector()
         {
            if( m_handle)
               exception::guard( [&]() { ipc::remove( m_handle.ipc());});
         }


         Device& device()
         {
            static Device singleton;
            return singleton;
         }
      } // inbound


      namespace outbound
      {
         namespace local
         {
            namespace
            {
               namespace open
               {
                  auto fifo( strong::ipc::id ipc)
                  {
                     auto file = ipc::local::ipc::path( ipc).string();
                     // we don't check if open succeeded, we let the later "send" take care of errors
                     auto descriptor = strong::file::descriptor::id{ ::open( file.data(), O_WRONLY | O_CLOEXEC)};

                     return Handle{ descriptor, ipc};
                  }

               } // open
            } // <unnamed>
         } // local
         
         Connector::Connector( strong::ipc::id ipc) 
            : m_handle{ local::open::fifo( ipc)}
         {
         }

      } // outbound


      bool exists( strong::ipc::id id)
      {
         return std::filesystem::exists( local::ipc::path( id));
      }


      bool remove( strong::ipc::id id)
      {
         return std::filesystem::remove( local::ipc::path( id));
      }

      bool remove( const process::Handle& owner)
      {
         return remove( owner.ipc);
      }

   } // common::communication::ipc
} // casual
