//!
//! casual
//!

#include "configuration/domain.h"
#include "configuration/file.h"

#include "common/exception.h"
#include "common/file.h"
#include "common/environment.h"
#include "common/algorithm.h"

#include "sf/archive/maker.h"

#include <algorithm>


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace domain
      {

         namespace local
         {
            namespace
            {

               namespace complement
               {

                  template< typename R, typename V>
                  void default_values( R& range, V&& value)
                  {
                     for( auto& element : range) { element += value;}
                  }

                  inline void default_values( Domain& domain)
                  {
                     default_values( domain.executables, domain.domain_default.executable);
                     default_values( domain.servers, domain.domain_default.server);
                     default_values( domain.services, domain.domain_default.service);
                  }

               } // complement

               void validate( const Domain& settings)
               {

               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = range::find( lhs, value);

                     if( found)
                     {
                        *found = std::move( value);
                     }
                     else
                     {
                        lhs.push_back( std::move( value));
                     }
                  }
               }

               template< typename D>
               Domain& append( Domain& lhs, D&& rhs)
               {
                  if( lhs.name.empty()) { lhs.name = std::move( rhs.name);}

                  local::replace_or_add( lhs.transaction.resources, rhs.transaction.resources);
                  local::replace_or_add( lhs.groups, rhs.groups);
                  local::replace_or_add( lhs.executables, rhs.executables);
                  local::replace_or_add( lhs.servers, rhs.servers);
                  local::replace_or_add( lhs.services, rhs.services);

                  lhs.gateway += std::move( rhs.gateway);
                  lhs.queue += std::move( rhs.queue);

                  return lhs;
               }

               Domain get( Domain domain, const std::string& file)
               {
                  //
                  // Create the reader and deserialize configuration
                  //
                  auto reader = sf::archive::reader::from::file( file);

                  reader >> CASUAL_MAKE_NVP( domain);

                  finalize( domain);

                  return domain;

               }

            } // unnamed
         } // local


         namespace domain
         {
            Default::Default()
            {
               server.instances.emplace( 1);
            }

         } // domain



         Domain& Domain::operator += ( const Domain& rhs)
         {
            return local::append( *this, rhs);
         }

         Domain& Domain::operator += ( Domain&& rhs)
         {
            return local::append( *this, std::move( rhs));
         }

         Domain operator + ( const Domain& lhs, const Domain& rhs)
         {
            auto result = lhs;
            result += rhs;
            return result;
         }

         void finalize( Domain& configuration)
         {
            //
            // Complement with default values
            //
            local::complement::default_values( configuration);

            //
            // Make sure we've got valid configuration
            //
            local::validate( configuration);

            configuration.transaction.finalize();
            configuration.gateway.finalize();
            configuration.queue.finalize();

         }


         Domain get( const std::vector< std::string>& files)
         {
            if( files.empty())
            {
               return persistent::get();
            }

            auto domain = range::accumulate( files, Domain{}, &local::get);

            {
               std::map< std::string, std::size_t> used;
               server::complement::Alias complement{ used};

               range::for_each( domain.servers, complement);
               range::for_each( domain.executables, complement);
            }

            return domain;

         }



         namespace persistent
         {
            Domain get()
            {
               auto configuration = file::persistent::domain();

               if( common::file::exists( configuration))
               {
                  return configuration::domain::get( { configuration});
               }
               else
               {
                  throw common::exception::invalid::File{ "failed to get persistent configuration file", CASUAL_NIP( configuration)};
               }
            }

            void save( const Domain& domain)
            {
               if( ! common::file::exists( directory::persistent()))
               {
                  common::directory::create( directory::persistent());
               }

               auto configuration = file::persistent::domain();

               auto archive = sf::archive::writer::from::file( configuration);
               archive << CASUAL_MAKE_NVP( domain);
            }

         } // persistent

      } // domain

   } // config
} // casual
