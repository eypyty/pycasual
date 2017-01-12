//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_

#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace scale
            {
               struct Executable : common::message::basic_message< common::message::Type::domain_scale_executable>
               {
                  Executable() = default;
                  Executable( std::initializer_list< std::size_t> executables) : executables{ std::move( executables)} {}

                  std::vector< std::size_t> executables;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & executables;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Executable& value);
               };
               static_assert( traits::is_movable< Executable>::value, "not movable");

            } // scale



            namespace process
            {
               namespace connect
               {
                  struct Request : basic_message< Type::domain_process_connect_request>
                  {
                     common::process::Handle process;
                     Uuid identification;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & process;
                        archive & identification;
                     })
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  struct Reply : basic_message< Type::domain_process_connect_reply>
                  {
                     enum class Directive : char
                     {
                        start,
                        singleton,
                        shutdown
                     };

                     Directive directive = Directive::start;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_type::marshal( archive);
                        archive & directive;
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // connect



               namespace termination
               {
                  using Registration = server::basic_id< Type::domain_process_termination_registration>;

                  using base_event = basic_message< Type::domain_process_termination_event>;

                  struct Event : base_event
                  {
                     Event() = default;
                     Event( common::process::lifetime::Exit death) : death{ std::move( death)} {}

                     common::process::lifetime::Exit death;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_event::marshal( archive);
                        archive & death;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Event& value);
                  };
                  static_assert( traits::is_movable< Event>::value, "not movable");

               } // termination

               namespace lookup
               {
                  using base_reqeust = server::basic_id< Type::domain_process_lookup_request>;
                  struct Request : base_reqeust
                  {
                     enum class Directive : char
                     {
                        wait,
                        direct
                     };

                     Uuid identification;
                     platform::pid::type pid = 0;
                     Directive directive = Directive::wait;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reqeust::marshal( archive);
                        archive & identification;
                        archive & pid;
                        archive & directive;
                     })

                     friend std::ostream& operator << ( std::ostream& out, Directive value);
                     friend std::ostream& operator << ( std::ostream& out, const Request& value);
                  };
                  static_assert( traits::is_movable< Request>::value, "not movable");

                  using base_reply = server::basic_id< Type::domain_process_lookup_reply>;

                  struct Reply : base_reply
                  {
                     Uuid identification;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reply::marshal( archive);
                        archive & identification;
                     })
                  };
                  static_assert( traits::is_movable< Reply>::value, "not movable");

               } // lookup

            } // process

            namespace server
            {
               namespace configuration
               {
                  using base_request = message::basic_request< message::Type::domain_server_configuration_request>;
                  struct Request : base_request
                  {

                  };

                  using base_reply = message::basic_request< message::Type::domain_server_configuration_reply>;
                  struct Reply : base_reply
                  {
                     std::vector< std::string> resources;
                     std::vector< std::string> restrictions;

                     CASUAL_CONST_CORRECT_MARSHAL(
                     {
                        base_reply::marshal( archive);
                        archive & resources;
                        archive & restrictions;
                     })
                  };

               } // configuration

            } // server

         } // domain

         namespace reverse
         {
            template<>
            struct type_traits< domain::process::lookup::Request> : detail::type< domain::process::lookup::Reply> {};

            template<>
            struct type_traits< domain::process::connect::Request> : detail::type< domain::process::connect::Reply> {};


            template<>
            struct type_traits< domain::server::configuration::Request> : detail::type< domain::server::configuration::Reply> {};

         } // reverse

      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_DOMAIN_H_
