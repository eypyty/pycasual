//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/environment/expand.h"
#include "common/environment/variable.h"

namespace casual
{
   namespace common
   {
      namespace environment
      {
         TEST( common_environment_string, empty_string)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE(  environment::expand( "").empty());
         }

         TEST( common_environment_string, invalid_variabel_declaration__expect_no_expansion)
         {
            common::unittest::Trace trace;

            EXPECT_TRUE( environment::expand( "foo${FOO") == "foo${FOO");
            EXPECT_TRUE( environment::expand( "foo$FOO") == "foo$FOO");
            EXPECT_TRUE( environment::expand( "foo$FOO}") == "foo$FOO}");
            EXPECT_TRUE( environment::expand( "$FOO") == "$FOO");
         }

         TEST( common_environment_string, valid_variabel_declaration__local_variable___expect_expansion)
         {
            common::unittest::Trace trace;

            const std::vector< environment::Variable> variables{
               { "FOO=foo"},
               { "BAR=bar"},
            };

            auto expand = [&variables]( auto string){ return environment::expand( string, variables);};

            EXPECT_TRUE( expand( "${FOO}") == "foo");
            EXPECT_TRUE( expand( "fuu${FOO}") == "fuufoo");
            EXPECT_TRUE( expand( "${FOO}fuu") == "foofuu");
            EXPECT_TRUE( expand( "f u u ${FOO} f u u") == "f u u foo f u u");
            EXPECT_TRUE( expand( "a${FOO}b${BAR}c") == "afoobbarc");
            EXPECT_TRUE( expand( "${FOO}${BAR}${FOO}${BAR}${FOO}${BAR}${FOO}${BAR}") == "foobarfoobarfoobarfoobar") << "expected: " << expand( "${FOO}${BAR}${FOO}${BAR}${FOO}${BAR}${FOO}${BAR}");
         }

      } // environment
   } // common
} // casual