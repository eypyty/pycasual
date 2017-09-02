//!
//! casual
//!

#ifndef CASUAL_COMMON_PROCESS_H_
#define CASUAL_COMMON_PROCESS_H_

#include "common/platform.h"
#include "common/file.h"

#include "common/algorithm.h"

#include "common/marshal/marshal.h"



//
// std
//
#include <string>
#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {
      struct Uuid;

      namespace process
      {

         //!
         //! @return the path of the current process
         //!
         const std::string& path();

         //!
         //! @return the basename of the current process
         //!
         const std::string& basename();


         //!
         //! Holds pid and ipc-queue for a given process
         //!
         struct Handle
         {
            using queue_handle = platform::ipc::id;

            Handle() = default;
            Handle( platform::process::id pid) : pid{ pid} {}
            Handle( platform::process::id pid, queue_handle queue) : pid( pid),  queue( queue)  {}

            platform::process::id pid;
            queue_handle queue;


            friend bool operator == ( const Handle& lhs, const Handle& rhs);
            inline friend bool operator != ( const Handle& lhs, const Handle& rhs) { return !( lhs == rhs);}
            friend bool operator < ( const Handle& lhs, const Handle& rhs);

            friend std::ostream& operator << ( std::ostream& out, const Handle& value);

            struct equal
            {
               struct pid
               {
                  pid( platform::process::id pid) : m_pid( pid) {}
                  bool operator() ( const Handle& lhs) { return lhs.pid == m_pid;}
               private:
                  platform::process::id m_pid;
               };
            };

            struct order
            {
               struct pid
               {
                  struct Ascending
                  {
                     bool operator() ( const Handle& lhs, const Handle& rhs) { return lhs.pid < rhs.pid;}
                  };
               };
            };

            inline explicit operator bool() const
            {
               return pid && queue;
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & pid;
               archive & queue;
            })
         };

         //!
         //! @return the process handle for current process
         //!
         Handle handle();

         //!
         //! @return process id (pid) for current process.
         //!
         platform::process::id id();

         inline platform::process::id id( const Handle& handle) { return handle.pid;}
         inline platform::process::id id( platform::process::id pid) { return pid;}

         namespace instance
         {

            namespace identity
            {
               namespace service
               {
                  const Uuid& manager();
               } // service

               namespace forward
               {
                  const Uuid& cache();
               } // forward

               namespace traffic
               {
                  const Uuid& manager();
               } // traffic

               namespace gateway
               {
                  const Uuid& manager();
               } // domain

               namespace queue
               {
                  const Uuid& manager();
               } // queue

               namespace transaction
               {
                  const Uuid& manager();
               } // transaction

            } // identity





            namespace fetch
            {
               enum class Directive : char
               {
                  wait,
                  direct
               };

               std::ostream& operator << ( std::ostream& out, Directive directive);

               Handle handle( const Uuid& identity, Directive directive = Directive::wait);

               //!
               //! Fetches the handle for a given pid
               //!
               //! @param pid
               //! @param directive if caller waits for the process to register or not
               //! @return handle to the process
               //!
               Handle handle( platform::process::id pid , Directive directive = Directive::wait);


            } // fetch




            void connect( const Uuid& identity, const Handle& process);
            void connect( const Uuid& identity);
            void connect( const Handle& process);
            void connect();

         } // instance


         //!
         //! @return the uuid for this process.
         //! used as a unique id over time
         //!
         const Uuid& uuid();

         //!
         //! Sleep for a while
         //!
         //! @throws exception::signal::* when a signal is received
         //!
         //! @param time numbers of microseconds to sleep
         //!
         void sleep( std::chrono::microseconds time);

         //!
         //! Sleep for an arbitrary duration
         //!
         //! Example:
         //! ~~~~~~~~~~~~~~~{.cpp}
         //! // sleep for 20 seconds
         //! process::sleep( std::chrono::seconds( 20));
         //!
         //! // sleep for 2 minutes
         //! process::sleep( std::chrono::minutes( 2));
         //! ~~~~~~~~~~~~~~~
         //!
         //! @throws exception::signal::* when a signal is received
         //!
         template< typename R, typename P>
         void sleep( std::chrono::duration< R, P> time)
         {
            sleep( std::chrono::duration_cast< std::chrono::microseconds>( time));
         }

         namespace pattern
         {
            struct Sleep
            {
               struct Pattern
               {

                  Pattern( std::chrono::microseconds time, platform::size::type quantity);

                  template< typename R, typename P>
                  Pattern( std::chrono::duration< R, P> time, platform::size::type quantity)
                   : Pattern{ std::chrono::duration_cast< std::chrono::microseconds>( time), quantity}
                  {}


                  bool done();

                  friend std::ostream& operator << ( std::ostream& out, const Pattern& value);

               private:
                  std::chrono::microseconds m_time;
                  platform::size::type m_quantity = 0;
               };
               
               Sleep( std::initializer_list< Pattern> pattern);

               bool operator () ();

               friend std::ostream& operator << ( std::ostream& out, const Sleep& value);

            private:
               std::vector< Pattern> m_pattern;
            };

         } // pattern




         //!
         //! Spawn a new application that path describes
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return process id of the spawned process
         //!
         platform::process::id spawn( const std::string& path, std::vector< std::string> arguments);


         platform::process::id spawn(
            std::string path,
            std::vector< std::string> arguments,
            std::vector< std::string> environment);

         //!
         //! Spawn a new application that path describes, and wait until it exit. That is
         //!  - spawn
         //!  - wait
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return exit code from the process
         //!
         int execute( const std::string& path, std::vector< std::string> arguments);


         //!
         //! Wait for a specific process to terminate.
         //!
         //! @return return code from process
         //!
         int wait( platform::process::id pid);


         //!
         //! Tries to terminate pids
         //!
         //! @return pids that did received the signal
         //!
         std::vector< platform::process::id> terminate( const std::vector< platform::process::id>& pids);

         //!
         //! Tries to terminate pid
         //!
         bool terminate( platform::process::id pid);


         //!
         //! Tries to shutdown the process, if it fails terminate signal will be signaled
         //!
         //! @param process to terminate
         //!
         void terminate( const Handle& process);



         //!
         //! ping a server that owns the @p queue
         //!
         //! @note will block
         //!
         //! @return the process handle
         //!
         Handle ping( platform::ipc::id queue);

         namespace lifetime
         {
            struct Exit
            {
               enum class Reason : char
               {
                  core,
                  exited,
                  stopped,
                  continued,
                  signaled,
                  unknown,
               };

               platform::process::id pid;
               int status = 0;
               Reason reason = Reason::unknown;

               explicit operator bool () const;

               //!
               //! @return true if the process life has ended
               //!
               bool deceased() const;


               friend bool operator == ( platform::process::id pid, const Exit& rhs);
               friend bool operator == ( const Exit& lhs, platform::process::id pid);
               friend bool operator < ( const Exit& lhs, const Exit& rhs);

               friend std::ostream& operator << ( std::ostream& out, const Reason& value);
               friend std::ostream& operator << ( std::ostream& out, const Exit& terminated);

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & pid;
                  archive & status;
                  archive & reason;
               })

            };

            std::vector< Exit> ended();


            //!
            //!
            //!
            std::vector< Exit> wait( const std::vector< platform::process::id>& pids);
            std::vector< Exit> wait( const std::vector< platform::process::id>& pids, std::chrono::microseconds timeout);


            //!
            //! Terminates and waits for the termination.
            //!
            //! @return the terminated l
            //!
            //
            std::vector< Exit> terminate( const std::vector< platform::process::id>& pids);
            std::vector< Exit> terminate( const std::vector< platform::process::id>& pids, std::chrono::microseconds timeout);

         } // lifetime

         namespace children
         {


            //!
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated callback is called
            //!
            //! @param callback the callback object
            //! @param pids to terminate
            //!
            template< typename C>
            void terminate( C&& callback, std::vector< platform::process::id> pids)
            {
               for( auto& death : lifetime::terminate( std::move( pids)))
               {
                  callback( death);
               }
            }


         } // children


      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */
