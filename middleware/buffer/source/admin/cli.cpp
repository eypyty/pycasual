//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/buffer/admin/cli.h"
#include "casual/buffer/internal/common.h"
#include "casual/buffer/internal/field.h"
#include "casual/cli/pipe.h"

#include "common/terminal.h"
#include <optional>
#include "common/communication/stream.h"

namespace casual
{
   using namespace common;

   namespace buffer
   {
      namespace admin
      {
         namespace cli
         {
            namespace local
            {
               namespace
               {
                  namespace field
                  {
                     auto to_human()
                     {
                        return argument::Option{
                           &cli::detail::field::to_human,
                           cli::detail::format::completion(),
                           { "--field-to-human"},
                           R"(reads from stdin and assumes a casual-fielded-buffer

and transform this to a human readable structure in the supplied format,
and prints this to stdout

@note: part of casual-pipe
@note: this is a 'casual-pipe' termination - no internal representation will be sent downstream)"
                        };
                     }

                     auto from_human()
                     {
                        return argument::Option{
                           &cli::detail::field::from_human,
                           cli::detail::format::completion(),
                           { "--field-from-human"},
                           R"(transform human readable fielded buffer to actual buffers

reads from stdin and assumes a human readable structure in the supplied format
for a casual-fielded-buffer, and transform this to an actual casual-fielded-buffer,
and forward this to stdout for other downstream in the pipeline to consume

@note: part of casual-pipe)"
                        }; 
                     }
                     
                  } // field

                  auto compose()
                  {
                     return argument::Option{
                        &cli::detail::compose,
                        cli::detail::buffer::types::completion(),
                        { "--compose"},
                        R"(reads 'binary' data from stdin and compose one actual buffer

with the supplied type, and forward this to stdout for other downstream 'components'
in the pipeline to consume

if no 'type' is provided, `X_OCTET/` is used

@note: part of casual-pipe)"
                     }; 
                  }

                  auto duplicate()
                  {
                     return argument::Option{
                        &cli::detail::duplicate,
                        { "--duplicate"},
                         R"(duplicates buffers read from stdin and send them downstream via stdout

`count` amount of times.

@note: part of casual-pipe)"
                     }; 
                  }

                  auto extract()
                  {
                     return argument::Option{
                        &cli::detail::extract,
                        { "--extract"},
                         R"(read the buffers from stdin and extract the payload and sends it to stdout

if --verbose is provided the type of the buffer will be sent to stderr.

@note: part of casual-pipe
@note: this is a 'casual-pipe' termination - no internal representation will be sent downstream)"
                     }; 
                  }
               } // <unnamed>
            } // local

            namespace detail
            {
               namespace field
               {
                  void from_human( const std::optional< std::string>& format)
                  {
                     Trace trace{ "buffer::admin::cli::detail::field::from_human"};

                     auto protocol = format.value_or( "");

                     casual::cli::message::Payload message;
                     message.payload = casual::buffer::field::internal::payload::stream( std::cin, protocol);

                     communication::stream::outbound::Device out{ std::cout};
                     communication::device::blocking::send( out, message);

                     casual::cli::pipe::done();
                  }

                  void to_human( const std::optional< std::string>& format)
                  {
                     Trace trace{ "buffer::admin::cli::detail::field::to_human"};

                     communication::stream::inbound::Device in{ std::cin};

                     auto protocol = format.value_or( "");

                     bool done = false;

                     auto handler = common::message::dispatch::handler( in, 
                        casual::cli::pipe::discard::handle::defaults(),
                        [&protocol]( casual::cli::message::Payload& message)
                        {
                           casual::buffer::field::internal::payload::stream( 
                              std::move( message.payload), 
                              std::cout, 
                              protocol);
                        },
                        casual::cli::pipe::handle::done( done)
                     );

                     common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                  }
               } // field

               void compose( const std::optional< std::string>& type)
               {
                  Trace trace{ "buffer::admin::cli::detail::compose"};

                  casual::cli::message::Payload message;

                  message.payload.type = type.value_or( common::buffer::type::x_octet());

                  while( std::cin.peek() != std::istream::traits_type::eof())
                     message.payload.memory.push_back( std::cin.get());

                  communication::stream::outbound::Device out{ std::cout};
                  communication::device::blocking::send( out, message);

                  casual::cli::pipe::done();
               }

               void duplicate( platform::size::type count)
               {
                  Trace trace{ "buffer::admin::cli::detail::duplicate"};

                  communication::stream::inbound::Device in{ std::cin};

                  bool done = false;

                  auto handler = common::message::dispatch::handler( in, 
                     casual::cli::pipe::forward::handle::defaults(),
                     [count]( const casual::cli::message::Payload& message)
                     {
                        communication::stream::outbound::Device out{ std::cout};

                        algorithm::for_n( count, [&message, &out]()
                        {
                           communication::device::blocking::send( out, message);
                        });
                     },
                     casual::cli::pipe::handle::done( done)
                  );

                  common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                  casual::cli::pipe::done();
               }

               void extract()
               {
                  Trace trace{ "buffer::admin::cli::detail::extract"};

                  communication::stream::inbound::Device in{ std::cin};

                  bool done = false;

                  auto handler = common::message::dispatch::handler( in, 
                     casual::cli::pipe::forward::handle::defaults(),
                     []( const casual::cli::message::Payload& message)
                     {
                        if( terminal::output::directive().verbose())
                           std::cerr << message.payload.type << '\n';

                        std::cout.write( message.payload.memory.data(), message.payload.memory.size());
                     },
                     casual::cli::pipe::handle::done( done)
                  );

                  common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);
               }

            } // detail

         } // cli

         struct CLI::Implementation
         {
            argument::Group options()
            {
               return argument::Group{ [](){}, { "buffer"}, "buffer related 'tools'",
                  cli::local::field::from_human(),
                  cli::local::field::to_human(),
                  cli::local::compose(),
                  cli::local::duplicate(),
                  cli::local::extract()
               };
            }
         };


         CLI::CLI() = default;
         CLI::~CLI() = default;

         common::argument::Group CLI::options() &
         {
            return m_implementation->options();
         }

      } // admin
   } // buffer
} // casual