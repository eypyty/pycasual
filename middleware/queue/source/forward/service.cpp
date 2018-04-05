//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/forward/common.h"
#include "queue/common/log.h"
#include "queue/api/queue.h"
#include "queue/common/transform.h"

#include "common/argument.h"
#include "common/exception/handle.h"

#include "common/buffer/pool.h"
#include "common/service/call/context.h"


namespace casual
{
   namespace queue
   {
      namespace forward
      {
         struct Settings
         {
            std::string queue;
            std::string service;
            common::optional< std::string> reply;

            auto tie() { return std::tie( queue, service, reply);}
         };

         struct Caller
         {
            Caller( std::string service, common::optional< std::string> reply) : m_service( std::move( service)), m_reply( std::move( reply)) {}

            void operator () ( queue::Message&& message)
            {
               Trace trace{ "queue::forward::Caller::operator()"};

               //
               // Prepare the xatmi-buffer
               //
               common::buffer::Payload payload{
                  std::move( message.payload.type),
                  std::move( message.payload.data)};

               log << "payload: " << payload << std::endl;

               try
               {
                  auto result = common::service::call::Context::instance().sync( m_service,
                        payload,
                        common::service::call::sync::Flag::no_time);

                  const auto& replyqueue = m_reply.value_or( message.attributes.reply);

                  if( ! replyqueue.empty())
                  {
                     queue::Message reply;
                     reply.payload.data = std::move( result.buffer.memory);
                     reply.payload.type = std::move( result.buffer.type);
                     queue::enqueue( replyqueue, reply);
                  }
               }
               catch( const common::service::call::Fail& exception)
               {
                  common::log::line( queue::log, "service call failed - rollback - ", exception);
               }
            }

         private:
            std::string m_service;
            common::optional< std::string> m_reply;
         };

         std::vector< forward::Task> tasks( Settings settings)
         {
            std::vector< forward::Task> tasks;
            tasks.emplace_back( std::move( settings.queue), Caller{ std::move( settings.service), std::move( settings.reply)});

            return tasks;
         }

         void start( Settings settings)
         {

            Dispatch dispatch( tasks( std::move( settings)));

            dispatch.execute();

         }

         int main( int argc, char **argv)
         {
            try
            {

               Settings settings;

               {
                  using namespace casual::common::argument;
                  Parse parse{ "queue forward to service",
                     Option( settings.tie(), {"-f", "--forward"}, "--forward  <queue> <service> [<reply>]")
                  };
                  parse( argc, argv);
               }

               start( std::move( settings));

            }
            catch( ...)
            {
               return common::exception::handle();

            }
            return 0;

         }

      } // forward
   } // queue
} // casual


int main( int argc, char **argv)
{
   return casual::queue::forward::main( argc, argv);
}




