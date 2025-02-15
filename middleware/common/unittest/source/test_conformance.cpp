//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"
#include "common/unittest/file.h"

#include "common/stream.h"
#include "common/algorithm.h"
#include "common/traits.h"
#include "common/signal.h"
#include "common/code/system.h"
#include "common/strong/type.h"
#include "common/environment.h"
#include "common/file.h"

#include <type_traits>



#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>


//
// Just a place to test C++ and POSIX conformance, or rather, casual's view on conformance
//


namespace casual
{

   namespace common
   {
      // static test of traits

      static_assert( traits::is::same_v< traits::remove_cvref_t< char const&>, char>, "traits::remove_cvref_t does not work...");

      static_assert( traits::is::string::like_v< decltype( "some string")>, "traits::is::string::like_v does not work...");

      static_assert( traits::is::string::like_v< char const (&)[7]>, "traits::is::string::like_v does not work...");
      static_assert( traits::is::string::like_v< char[ 20]>, "traits::is::string::like_v does not work...");

      static_assert( ! traits::is::string::like_v< char* [ 20]>, "traits::is::string::like_v does not work...");


      static_assert( traits::is::iterable_v< char[ 20]>, "traits::is::iterable does not work...");

      static_assert( ! traits::is::any_v< char, unsigned char, signed char>, "traits::is::any_v does not work...");
      static_assert( traits::is::any_v< char, unsigned char, signed char, char>, "traits::is_any does not work...");

      static_assert( traits::is::tuple_v< std::pair< int, long>>, "traits::is::tuple_v does not work...");

      template< int... values>
      constexpr auto size_of_parameter_pack()
      {
         return sizeof...( values);
      }

      static_assert( size_of_parameter_pack<>() == 0);
      static_assert( size_of_parameter_pack< 1, 2, 3>() == 3);

      namespace detail
      {
         template< typename F>
         auto switch_fallthrough( int value, F&& function)
         {
            switch( value)
            {
               case 1: function( value); // case is doing stuff
               [[fallthrough]];          // hence, need explict fallthrough
               case 2:                   // case is not doing stuff, no fallthrough needed.
               case 3: return 3;
            }
            return 0;
         }
         
      } // detail

      


      TEST( common_conformance, fallthrough)
      {
         int value{};
         auto function = [&value]( auto v){ value = v;};

         EXPECT_TRUE( detail::switch_fallthrough( 1, function) == 3);
         EXPECT_TRUE( value == 1);
      }
      
      TEST( common_conformance, filesystem_path__environment_expansion)
      {
         auto path = std::filesystem::path{ "${FOO}/${BAR}"};
         auto range = range::make( path);
         EXPECT_TRUE( range.size() == 2);
      }

      TEST( common_conformance, struct_with_pod_attributes__is_pod)
      {
         struct POD
         {
            int int_value;
         };

         EXPECT_TRUE( std::is_standard_layout< POD>::value);
      }

      TEST( common_conformance, struct_with_member_function__is_pod)
      {
         struct POD
         {
            int func1() { return 42;}
         };

         EXPECT_TRUE( std::is_standard_layout< POD>::value);
      }


      TEST( common_conformance, is_floating_point__is_signed)
      {

         EXPECT_TRUE( std::is_signed< float>::value);
         EXPECT_TRUE( std::is_floating_point< float>::value);

         EXPECT_TRUE( std::is_signed< double>::value);
         EXPECT_TRUE( std::is_floating_point< double>::value);

      }





      struct some_functor
      {
         void operator () ( const double& value)  {}

         int test;
      };


      TEST( common_conformance, get_functor_argument_type)
      {

         EXPECT_TRUE( traits::function< some_functor>::arguments() == 1);

         using argument_type = typename traits::function< some_functor>::argument< 0>::type;

         auto is_same = std::is_same< const double&, argument_type>::value;
         EXPECT_TRUE( is_same);

      }


      TEST( common_conformance, get_function_argument_type)
      {
         using function_1 = std::function< void( double&)>;

         EXPECT_TRUE( traits::function< function_1>::arguments() == 1);

         using argument_type = typename traits::function< function_1>::argument< 0>::type;

         auto is_same = std::is_same< double&, argument_type>::value;
         EXPECT_TRUE( is_same);

      }



      long some_function( const std::string& value) { return 1;}

      TEST( common_conformance, get_free_function_argument_type)
      {
         using traits_type = traits::function< decltype( &some_function)>;

         EXPECT_TRUE( traits_type::arguments() == 1);

         {
            using argument_type = typename traits_type::argument< 0>::type;

            auto is_same = std::is_same< const std::string&, argument_type>::value;
            EXPECT_TRUE( is_same);
         }

         {
            using result_type = typename traits_type::result_type;

            auto is_same = std::is_same< long, result_type>::value;
            EXPECT_TRUE( is_same);
         }
      }


      TEST( common_conformance, search)
      {
         std::string source{ "some string to search in"};
         std::string to_find{ "some"};


         EXPECT_TRUE( std::search( std::begin( source), std::end( source), std::begin( to_find), std::end( to_find)) != std::end( source));
      }


      TEST( common_conformance, posix_spawnp)
      {
         common::unittest::Trace trace;

         // We don't want any sig-child
         signal::thread::scope::Block block{ { code::signal::child}};


         auto pids = algorithm::generate_n< 30>( []()
         {
            pid_t pid{};
            std::vector< const char*> arguments{ "sleep", "20", nullptr};

            auto current = environment::variable::native::current();

            auto environment = algorithm::transform( current, []( auto& value){ return value.data();});
            environment.push_back( nullptr);

            if( posix_spawnp(
                  &pid,
                  "sleep",
                  nullptr,
                  nullptr,
                  const_cast< char* const*>( arguments.data()),
                  const_cast< char* const*>( environment.data())) == 0)
            {
               return strong::process::id{ pid};
            }
            return strong::process::id{};
         });

         {
            auto signaled = decltype( pids){};

            for( auto pid : pids)
               if( kill( pid.value(), SIGINT) == 0)
                  signaled.push_back( pid);

            EXPECT_TRUE( signaled == pids);
         }

         {
            auto terminated = decltype( pids){};

            for( auto pid : pids)
            {
               auto result = waitpid( pid.value(), nullptr, 0);

               EXPECT_TRUE( result == pid.value()) << string::compose( "result: ", result, " - pid: ", pid, " - errno: ", common::code::system::last::error());

               if( result == pid.value())
                  terminated.push_back( pid);
            }
            EXPECT_TRUE( terminated == pids) << string::compose( "terminated: ", terminated, ", pids: ", pids);
         }
      }



      namespace local
      {
         namespace
         {
            int handle_exception()
            {
               try 
               {
                  throw;
               }
               catch( int value)
               {
                  return value;
               }
               catch( ...)
               {
                  return 0;
               }
            }

            void handle_exception_rethrow( int value)
            {
               try 
               {
                  throw value;
               }
               catch( ...)
               {
                  EXPECT_TRUE( handle_exception() == value);
                  throw;
               }
            }
            
         } // <unnamed>
      } // local

      TEST( common_conformance, nested_handled_exception_retrown)
      {
         try 
         {
            local::handle_exception_rethrow( 42);
         }
         catch( ...)
         {
            EXPECT_TRUE( local::handle_exception() == 42);
         }
      }


      TEST( common_conformance_filesystem, directory_symlink)
      {
         common::unittest::Trace trace;

         namespace fs = std::filesystem;
         unittest::directory::temporary::Scoped base;
         fs::path base_path{ base.path()};
         ASSERT_TRUE( fs::create_directories( base_path / "origin"));

         fs::create_directory_symlink( base_path / "origin", base_path / "link");

         // already exists -> gives false
         EXPECT_TRUE( ! fs::create_directories(  base_path / "origin"));

         EXPECT_TRUE( fs::exists( base_path / "link"));
         
         // fails hard, since 'link' is a link... Why?!! 
         // EXPECT_TRUE( fs::create_directories(  base_path / "link"));

         EXPECT_NO_THROW({
            // "helper" that takes care of corner cases
            common::directory::create( base_path / "link");
         });

         // check if we can create a sub-directory in the linked directory
         EXPECT_TRUE( fs::create_directories(  base_path / "link/subdir"));

         {
            std::ofstream out{ base_path / "link/file"};
            EXPECT_TRUE( out);
         }

      }


      /*
       * generates error with -Werror=return-type
       *
      namespace local
      {
         namespace
         {
            bool ommit_return()
            {

            }
         } // <unnamed>
      } // local


      TEST( common_conformance, ommitt_return)
      {
         EXPECT_TRUE( local::ommit_return());
      }
      */



      


   } // common
} // casual
