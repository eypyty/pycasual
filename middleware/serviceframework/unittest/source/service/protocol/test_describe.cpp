//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "serviceframework/service/protocol/describe.h"
#include "serviceframework/service/protocol.h"
#include "common/serialize/line.h"

#include <map>
#include <vector>

namespace casual
{
   namespace serviceframework
   {
      TEST( serviceframework_service_model, instansiate)
      {
         common::unittest::Trace trace;

         service::Model model;

         service::Model::Type type;
         type.attribues.push_back( type);
      }


      TEST( serviceframework_service_archive, basic_serialization)
      {
         common::unittest::Trace trace;

         service::Model model;

         service::protocol::describe::Wrapper writer{ model.arguments.input};

         std::string some_string;
         long some_long = 0;

         writer << CASUAL_NAMED_VALUE( some_string);
         writer << CASUAL_NAMED_VALUE( some_long);

         ASSERT_TRUE( model.arguments.input.size() == 2);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_string");
         EXPECT_TRUE( model.arguments.input.at( 0).category == service::model::type::Category::string);
         EXPECT_TRUE( model.arguments.input.at( 1).role == "some_long");
         EXPECT_TRUE( model.arguments.input.at( 1).category == service::model::type::Category::integer);
      }

      TEST( serviceframework_service_archive, container_serialization)
      {
         common::unittest::Trace trace;

         service::Model model;

         service::protocol::describe::Wrapper writer{ model.arguments.input};

         std::vector< std::string> some_strings;
         std::vector< long> some_longs;

         writer << CASUAL_NAMED_VALUE( some_strings);
         writer << CASUAL_NAMED_VALUE( some_longs);

         ASSERT_TRUE( model.arguments.input.size() == 2);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_strings");
         EXPECT_TRUE( model.arguments.input.at( 0).category == service::model::type::Category::container);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.front().category == service::model::type::Category::string);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.front().role.empty());
         EXPECT_TRUE( model.arguments.input.at( 1).role == "some_longs");
         EXPECT_TRUE( model.arguments.input.at( 1).category == service::model::type::Category::container);
         ASSERT_TRUE( model.arguments.input.at( 1).attribues.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 1).attribues.front().category == service::model::type::Category::integer);
         EXPECT_TRUE( model.arguments.input.at( 1).attribues.front().role.empty());
      }

      namespace local
      {
         namespace
         {
            struct Composite
            {
               std::string some_string;
               platform::binary::type some_binary;
               long some_long = 0;
               bool some_bool = true;
               double some_double = 0.0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( some_string);
                  CASUAL_SERIALIZE( some_binary);
                  CASUAL_SERIALIZE( some_long);
                  CASUAL_SERIALIZE( some_bool);
                  CASUAL_SERIALIZE( some_double);
               })



            };

            bool operator < ( const Composite& lhs, const Composite& rhs) { return true;}

            namespace composit
            {
               void validate( const service::Model::Type& type)
               {
                  ASSERT_TRUE( type.attribues.size() == 5);
                  EXPECT_TRUE( type.attribues.at( 0).role == "some_string");
                  EXPECT_TRUE( type.attribues.at( 0).category == service::model::type::Category::string);
                  EXPECT_TRUE( type.attribues.at( 1).role == "some_binary");
                  EXPECT_TRUE( type.attribues.at( 1).category == service::model::type::Category::binary);
                  EXPECT_TRUE( type.attribues.at( 2).role == "some_long");
                  EXPECT_TRUE( type.attribues.at( 2).category == service::model::type::Category::integer);
                  EXPECT_TRUE( type.attribues.at( 3).role == "some_bool");
                  EXPECT_TRUE( type.attribues.at( 3).category == service::model::type::Category::boolean);
                  EXPECT_TRUE( type.attribues.at( 4).role == "some_double");
                  EXPECT_TRUE( type.attribues.at( 4).category == service::model::type::Category::floatingpoint);
               }

            } // composit

         } // <unnamed>
      } // local


      TEST( serviceframework_service_archive, composite_serialization)
      {
         common::unittest::Trace trace;

         service::Model model;
         service::protocol::describe::Wrapper writer{ model.arguments.input};

         local::Composite some_composite;

         writer << CASUAL_NAMED_VALUE( some_composite);

         ASSERT_TRUE( model.arguments.input.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_composite");
         EXPECT_TRUE( model.arguments.input.at( 0).category == service::model::type::Category::composite) << model.arguments.input.at( 0).category;

         local::composit::validate( model.arguments.input.at( 0));
      }

      namespace local
      {
         namespace
         {
            struct NestedComposite
            {
               std::vector< long> some_longs;
               std::vector< local::Composite> some_composites;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( some_longs);
                  CASUAL_SERIALIZE( some_composites);
               })
            };

         } // <unnamed>
      } // local

      TEST( serviceframework_service_archive, complex_serialization)
      {
         common::unittest::Trace trace;

         service::Model model;
         service::protocol::describe::Wrapper writer{ model.arguments.input};

         std::map< local::Composite, local::NestedComposite> complex;
         writer << CASUAL_NAMED_VALUE( complex);

         ASSERT_TRUE( model.arguments.input.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "complex");
         EXPECT_TRUE( model.arguments.input.at( 0).category == service::model::type::Category::container);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.size() == 1);

         {
            auto& map = model.arguments.input.at( 0).attribues.at( 0);

            EXPECT_TRUE( map.role.empty());
            EXPECT_TRUE( map.category == service::model::type::Category::container);
            ASSERT_TRUE( map.attribues.size() == 2) << CASUAL_NAMED_VALUE( model);

            {
               auto& first = map.attribues.at( 0);
               local::composit::validate( first);
            }

            {
               auto& second = map.attribues.at( 1);
               ASSERT_TRUE( second.attribues.size() == 2) << CASUAL_NAMED_VALUE( model);

               {
                  auto& some_longs = second.attribues.at( 0);

                  EXPECT_TRUE( some_longs.category == service::model::type::Category::container);
                  EXPECT_TRUE( some_longs.role == "some_longs");

                  ASSERT_TRUE( some_longs.attribues.size() == 1);
                  EXPECT_TRUE( some_longs.attribues.at( 0).role.empty());
                  EXPECT_TRUE( some_longs.attribues.at( 0).category == service::model::type::Category::integer);

               }

               {
                  auto& some_composites = second.attribues.at( 1);

                  EXPECT_TRUE( some_composites.category == service::model::type::Category::container);
                  EXPECT_TRUE( some_composites.role == "some_composites");
                  ASSERT_TRUE( some_composites.attribues.size() == 1) << CASUAL_NAMED_VALUE( some_composites);


                  local::composit::validate( some_composites.attribues.at( 0));
               }
            }

         }
      }

   } // serviceframework

} // casual
