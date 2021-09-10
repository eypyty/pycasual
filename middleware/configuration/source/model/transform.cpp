//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/model/transform.h"
#include "configuration/user/environment.h"
#include "configuration/common.h"

#include "common/service/type.h"
#include "common/chronology.h"


namespace casual
{
   using namespace common;
   namespace configuration::model
   {
      namespace local
      {
         namespace
         {

            template< typename T>
            T empty_if_null( std::optional< T> value)
            {
               if( value)
                  return std::move( value.value());
               return {};
            }

            auto environment( const user::Environment& environment)
            {
               domain::Environment result;
               result.variables = user::environment::transform( user::environment::fetch( environment));
               return result;
            }

            auto transaction( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::transaction"};

               transaction::Model result;

               if( ! domain.transaction)
                  return result;

               auto& transaction = domain.transaction.value();

               result.log = transaction.log;

               result.resources = common::algorithm::transform( transaction.resources, []( const auto& resource)
               {
                  transaction::Resource result;

                  result.name = resource.name;
                  result.key = resource.key.value_or( "");
                  result.instances = resource.instances.value_or( 0);
                  result.note = resource.note.value_or( "");
                  result.openinfo = resource.openinfo.value_or( "");
                  result.closeinfo = resource.closeinfo.value_or( "");

                  return result;
               });

               // extract all explicit and implicit (via groups) resources for every server, if any
               {
                  auto append_resources = [&]( auto& alias, auto resources)
                  {
                     if( resources.empty())
                        return;

                     if( auto found = algorithm::find( result.mappings, alias))
                        algorithm::append_unique( std::move( resources), found->resources);
                     else 
                     {
                        auto& mapping = result.mappings.emplace_back();
                        mapping.alias = alias;
                        mapping.resources = std::move( resources);
                        algorithm::trim( mapping.resources, algorithm::unique( algorithm::sort( mapping.resources)));
                     }   
                  };

                  auto extract_group_resources = [&]( auto& memberships)
                  {
                     return algorithm::accumulate( memberships, std::vector< std::string>{}, [&]( auto result, auto& membership)
                     {
                        if( auto found = algorithm::find( domain.groups, membership))
                           if( found->resources)
                              algorithm::append( found->resources.value(), result);
                        
                        return result;
                     });
                  };

                  auto server_resources = [&]( auto& server)
                  {
                     auto& alias = server.alias.value();
                     if( server.resources)
                        append_resources( alias, server.resources.value());

                     if( server.memberships)
                        append_resources( alias, extract_group_resources( server.memberships.value()));
                  };

                  algorithm::for_each( domain.servers, server_resources);
               }

               return result;
            }

            auto domain( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::domain"};

               domain::Model result;
               
               result.name = domain.name;

               if( domain.environment)
                  result.environment = local::environment( domain.environment.value());

               // groups
               result.groups = common::algorithm::transform( domain.groups, []( auto& group)
               {
                  domain::Group result;
                  result.name = group.name;
                  result.note = group.note.value_or( "");
                  result.dependencies = group.dependencies.value_or( result.dependencies);
                  return result;
               });

               auto transform_executable = []( auto result)
               {
                  using result_type = decltype( result);

                  return []( auto& value)
                  {
                     result_type result;
                     result.alias = value.alias.value_or( "");
                     result.arguments = value.arguments.value_or( result.arguments);
                     result.instances = value.instances.value_or( result.instances);
                     result.note = value.note.value_or( "");
                     result.path = value.path;
                     result.lifetime.restart = value.restart.value_or( result.lifetime.restart);
                     result.memberships = value.memberships.value_or( result.memberships);

                     // TOOD performance: worst case we'd read env-files a lot of times...
                     if( value.environment)
                        result.environment = local::environment( value.environment.value());
                     return result;
                  };
               };

               result.executables = common::algorithm::transform( domain.executables, transform_executable( domain::Executable{}));
               result.servers = common::algorithm::transform( domain.servers, transform_executable( domain::Server{}));

               return result;
            }

            namespace detail::execution
            {
               auto timeout( const std::optional<configuration::user::service::Execution>& execution)
               {
                  configuration::model::service::Timeout result;
                  if( execution && execution.value().timeout)
                  {
                     if ( execution.value().timeout.value().duration)
                     {
                        result.duration = 
                           common::chronology::from::string( execution.value().timeout.value().duration.value());
                     }
                     if ( execution.value().timeout.value().contract)
                     {
                        result.contract = common::service::execution::timeout::contract::transform( execution.value().timeout.value().contract.value());
                     }
                  }
                  return result;
               }
               
            }

            auto service( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::service"};

               namespace contract = common::service::execution::timeout::contract;

               service::Model result;

               if( domain.defaults && domain.defaults.value().service && domain.defaults.value().service.value().timeout)
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.default.service.timeout are deprecated - use domain.default.service.execution.duration instead");
                  result.timeout.duration = common::chronology::from::string( domain.defaults.value().service.value().timeout.value());
               }

               if( domain.service && domain.service.value().execution && domain.service.value().execution.value().timeout)
               {
                  const auto& timeout = domain.service.value().execution.value().timeout;
                  if( timeout.value().duration)
                     result.timeout.duration = common::chronology::from::string( timeout.value().duration.value());

                  if( timeout.value().contract)
                     result.timeout.contract = contract::transform( timeout.value().contract.value());
               }

               result.services = common::algorithm::transform( domain.services, []( const auto& service)
               {
                  service::Service result;

                  result.name = service.name;
                  result.routes = service.routes.value_or( result.routes);
                  if( service.timeout)
                  {
                     log::line( log::category::warning, code::casual::invalid_configuration, " domain.service.timeout are deprecated - use domain.service.execution.duration instead");
    
                     result.timeout.duration = common::chronology::from::string( service.timeout.value());
                  }

                  if( service.execution)
                     result.timeout = detail::execution::timeout( service.execution);

                  return result;
               });

               result.restrictions = algorithm::transform_if( domain.servers, []( auto& service)
               {
                  service::Restriction result;
                  result.alias = service.alias.value();
                  result.services = service.restrictions.value();

                  return result;
               }, []( auto& service)
               {
                  return service.restrictions && ! service.restrictions.value().empty();
               });

               return result;
            }

            auto gateway( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::gateway"};

               gateway::Model result;

               if( ! domain.gateway)
                  return result;

               auto& gateway = domain.gateway.value();

               // first we take care of deprecated stuff

               if( gateway.listeners && ! gateway.listeners.value().empty())
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.listeners are deprecated - use domain.gateway.inbounds");

                  gateway::inbound::Group group;
                  group.connect = decltype( group.connect)::regular;
                  group.connections = common::algorithm::transform( gateway.listeners.value(), []( const auto& listener)
                  {
                     gateway::inbound::Connection result;
                     result.note = listener.note.value_or( "");
                     result.address = listener.address;
                     return result;
                  });


                  group.limit = algorithm::accumulate( gateway.listeners.value(), gateway::inbound::Limit{}, [&]( auto current, auto& listener)
                  {
                     if( ! listener.limit)
                        return current;

                     auto size = listener.limit.value().size.value_or( 0);
                     auto messages = listener.limit.value().messages.value_or( 0);

                     if( size > 0 && current.size > size)
                        current.size = size;

                     if( messages > 0 && current.messages > messages)
                        current.messages = messages;

                     return current;
                  });

                  result.inbound.groups.push_back( std::move( group));
               }

               if( gateway.connections && ! gateway.connections.value().empty())
               {
                  log::line( log::category::warning, code::casual::invalid_configuration, " domain.gateway.connections are deprecated - use domain.gateway.outbounds");

                  result.outbound.groups = common::algorithm::transform( gateway.connections.value(), []( const auto& value)
                  {
                     gateway::outbound::Group result;
                     result.connect = decltype( result.connect)::regular;
                     result.note = value.note.value_or( "");
                     gateway::outbound::Connection connection;
                     
                     connection.note = result.note;
                     connection.address = value.address;
                  
                     if( value.services)
                        connection.services = value.services.value();
                     if( value.queues)
                        connection.queues = value.queues.value();

                     result.connections.push_back( std::move( connection));

                     return result;
                  });
               }

               auto append_inbounds = []( auto& source, auto& target, auto connect)
               { 
                  if( ! source)
                     return;

                  algorithm::transform( source.value().groups, std::back_inserter( target), [connect]( auto& source)
                  {
                     gateway::inbound::Group result;
                     result.alias = source.alias.value_or( "");
                     result.note = source.note.value_or( "");
                     result.connect = connect;

                     if( source.limit)
                     {
                        result.limit.size = source.limit.value().size.value_or( result.limit.size);
                        result.limit.messages = source.limit.value().messages.value_or( result.limit.messages);
                     }

                     result.connections = algorithm::transform( source.connections, []( auto& connection)
                     {
                        gateway::inbound::Connection result;
                        result.note = connection.note.value_or( "");
                        result.address = connection.address;
                        
                        if( connection.discovery && connection.discovery.value().forward)
                           result.discovery = decltype( result.discovery)::forward;

                        return result;
                     });

                     return result;
                  });
               };

               auto append_outbounds = []( auto& source, auto& target, auto connect)
               { 
                  if( ! source)
                     return;

                  algorithm::transform( source.value().groups, std::back_inserter( target), [connect]( auto& source)
                  {
                     gateway::outbound::Group result;
                     result.alias = source.alias.value_or( "");
                     result.note = source.note.value_or( "");
                     result.connect = connect;
                     result.connections = algorithm::transform( source.connections, []( auto& connection)
                     {
                        gateway::outbound::Connection result;
                        result.note = connection.note.value_or( "");
                        result.address = connection.address;
                        result.services = local::empty_if_null( std::move( connection.services));
                        result.queues = local::empty_if_null( std::move( connection.queues));
                        return result;
                     });

                     return result;
                  });
               };

               append_inbounds( gateway.inbound, result.inbound.groups, configuration::model::gateway::connect::Semantic::regular);
               append_outbounds( gateway.outbound, result.outbound.groups, configuration::model::gateway::connect::Semantic::regular);

               if( gateway.reverse)
               {
                  append_inbounds( gateway.reverse.value().inbound, result.inbound.groups, configuration::model::gateway::connect::Semantic::reversed);
                  append_outbounds( gateway.reverse.value().outbound, result.outbound.groups, configuration::model::gateway::connect::Semantic::reversed);
               }

               return result;
            }

            auto queue( const configuration::user::Domain& domain)
            {
               Trace trace{ "configuration::model::local::queue"};

               queue::Model result;

               if( ! domain.queue)
                  return result;

               auto& source = domain.queue.value();

               result.note = source.note.value_or( "");

               if( source.groups)
               {
                  result.groups = common::algorithm::transform( source.groups.value(), []( auto& group)
                  {
                     queue::Group result;

                     result.alias = group.alias.value_or( "");
                     result.note = group.note.value_or( "");
                     result.queuebase = group.queuebase.value_or( "");

                     common::algorithm::transform( group.queues, result.queues, []( auto& queue)
                     {
                        model::queue::Queue result;

                        result.name = queue.name;
                        result.note = queue.note.value_or("");
                        if( queue.retry)
                        {
                           auto& retry = queue.retry.value();
                           if( retry.count)
                              result.retry.count = retry.count.value();
                           if( retry.delay)
                              result.retry.delay = common::chronology::from::string( retry.delay.value());
                        }

                        return result;
                     });
                     return result;
                  });
               }
               
               
               if( source.forward && source.forward.value().groups)
               {
                  result.forward.groups = algorithm::transform( source.forward.value().groups.value(), []( auto& group)
                  {
                     queue::forward::Group result;
                     result.alias = group.alias.value_or( "");

                     auto set_base_forward = []( auto& source, auto& target)
                     {
                        target.alias = source.alias.value_or( "");
                        target.source = source.source;
                        if( source.instances)
                           target.instances = source.instances.value();

                        target.note = source.note.value_or( "");
                     };

                     if( group.services)
                     {
                        result.services = algorithm::transform( group.services.value(), [&set_base_forward]( auto& service)
                        {
                           configuration::model::queue::forward::Service result;

                           set_base_forward( service, result);
                           result.target.service = service.target.service;

                           if( service.reply)
                           {
                              configuration::model::queue::forward::Service::Reply reply;
                              reply.queue = service.reply.value().queue;

                              if( service.reply.value().delay)
                                 reply.delay =  common::chronology::from::string( service.reply.value().delay.value());
            
                              result.reply = std::move( reply);
                           }

                           return result;
                        });
                     }

                     if( group.queues)
                     {
                        result.queues = algorithm::transform( group.queues.value(), [&set_base_forward]( auto& queue)
                        {
                           configuration::model::queue::forward::Queue result;

                           set_base_forward( queue, result);

                           result.target.queue = queue.target.queue;
                           if( queue.target.delay)
                              result.target.delay = common::chronology::from::string( queue.target.delay.value());

                           return result;
                        });
                     }
                     
                     return result;
                  });
               }
         
               return result;
            }

            namespace model
            {
               namespace detail
               {
                  template< typename T>
                  auto empty( const T& value, traits::priority::tag< 0>) -> decltype( value == 0)
                  {
                     return value == 0;
                  }

                  template< typename T>
                  auto empty( const T& value, traits::priority::tag< 1>) -> decltype( value.empty())
                  {
                     return value.empty();
                  }
               } // detail

               template< typename T>
               auto null_if_empty( T&& value)
               {
                  using optional_t = std::optional< std::decay_t< T>>;

                  if( detail::empty( value, traits::priority::tag< 1>{}))
                     return optional_t{};

                  return optional_t{ std::forward< T>( value)};
               };

               auto domain( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::domain"};

                  configuration::user::Domain result;

                  result.name = model.domain.name;

                  if( model.service.timeout.duration)
                     result.defaults.emplace().service.emplace().timeout = chronology::to::string( model.service.timeout.duration.value());

                  if( ! model.domain.environment.variables.empty())
                     result.environment.emplace().variables = configuration::user::environment::transform( model.domain.environment.variables);

                  auto assign_executable = []( auto& source, auto& target)
                  {
                     target.alias = source.alias;
                     target.path = source.path;
                     target.memberships = source.memberships;
                     target.restart = source.lifetime.restart;
                     target.arguments = source.arguments;
                     target.instances = source.instances;
                     target.note = null_if_empty( source.note);

                     
                     if( ! source.environment.variables.empty())
                     {
                        configuration::user::Environment environment;
                        environment.variables = user::environment::transform( source.environment.variables);
                        target.environment = std::move( environment);
                     }
                        
                  };

                  result.servers = algorithm::transform( model.domain.servers, [&]( auto& value)
                  {
                     configuration::user::domain::Server result;
                     assign_executable( value, result);
                     
                     if( auto found = algorithm::find( model.transaction.mappings, value.alias))
                        result.resources = null_if_empty( found->resources);

                     if( auto found = algorithm::find( model.service.restrictions, value.alias))
                        result.restrictions = null_if_empty( found->services);

                     return result;
                  });
               
                  result.executables = algorithm::transform( model.domain.executables, [&]( auto& value)
                  {
                     configuration::user::domain::Executable result;
                     assign_executable( value, result);
                     return result;
                  });

                  result.groups = algorithm::transform( model.domain.groups, []( auto& value)
                  {
                     configuration::user::domain::Group result;
                     result.name = value.name;
                     result.note = null_if_empty( value.note);
                     result.dependencies = null_if_empty( value.dependencies);

                     return result;
                  });

                  return result;
               }
               
               namespace detail
               {
                  auto execution( const configuration::model::service::Service& service)
                  {
                     configuration::user::service::execution::Timeout timeout;                  
                     if( service.timeout.duration)
                        timeout.duration = chronology::to::string( service.timeout.duration.value());

                     timeout.contract = common::service::execution::timeout::contract::transform( service.timeout.contract);

                     std::optional<configuration::user::service::Execution> result;
                     if( timeout.duration || timeout.contract)
                        result.emplace().timeout = std::move( timeout);

                     return result;                     
                  }

               }

               auto service( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::service"};

                  return algorithm::transform( model.service.services, []( auto& service)
                  {
                     user::service::Service result;

                     result.routes = null_if_empty( service.routes);
                     result.name = service.name;
                     result.execution = detail::execution( service);

                     return result;
                  });

               }

               auto transaction( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::transaction"};

                  user::transaction::Manager result;

                  result.log = model.transaction.log;
                  result.resources = algorithm::transform( model.transaction.resources, []( auto& value)
                  {
                     user::transaction::Resource result;

                     result.name = value.name;
                     result.key = null_if_empty( value.key);
                     result.openinfo = null_if_empty( value.openinfo);
                     result.closeinfo = null_if_empty( value.closeinfo);
                     result.note  = null_if_empty( value.note);
                     result.instances = value.instances;

                     return result;
                  });

                  return result;
               }

               auto gateway( configuration::model::gateway::Model model)
               {
                  Trace trace{ "configuration::model::local::model::gateway"};

                  user::gateway::Manager result;

                  auto transform_inbound = []( auto& value) 
                  {
                     configuration::user::gateway::inbound::Group result;
                     result.alias = value.alias;
                     result.note = null_if_empty( value.note);
                     
                     if( value.limit.size > 0 || value.limit.messages > 0)
                     {
                        user::gateway::inbound::Limit limit;
                        limit.messages = null_if_empty( value.limit.messages);
                        limit.size = null_if_empty( value.limit.size);
                        result.limit = std::move( limit);
                     };

                     result.connections = algorithm::transform( value.connections, []( auto& value)
                     {
                        configuration::user::gateway::inbound::Connection result;
                        result.address = value.address;
                        result.note = null_if_empty( value.note);

                        if( value.discovery == decltype( value.discovery)::forward)
                           result.discovery = configuration::user::gateway::inbound::Discovery{ true};


                        return result;
                     });
                     return result;
                  };

                  auto transform_outbound = []( auto& value) 
                  {
                     configuration::user::gateway::outbound::Group result;
                     result.alias = value.alias;
                     result.note = null_if_empty( value.note);
                     result.connections = algorithm::transform( value.connections, []( auto& value)
                     {
                        configuration::user::gateway::outbound::Connection result;
                        result.address = value.address;
                        result.note = null_if_empty( value.note);
                        result.services = null_if_empty( value.services);
                        result.queues = null_if_empty( value.queues);
                        return result;
                     });
                     return result;
                  };

                  auto is_reversed = []( auto& value){ return value.connect == decltype( value.connect)::reversed;};
                  
                  auto reverse = configuration::user::gateway::Reverse{};


                  // inbounds
                  {
                     auto [ reversed, regular] = algorithm::stable::partition( model.inbound.groups, is_reversed);

                     if( reversed)
                     {
                        configuration::user::gateway::Inbound inbound;
                        inbound.groups = algorithm::transform( reversed, transform_inbound);
                        reverse.inbound = std::move( inbound);
                     }

                     if( regular)
                     {
                        configuration::user::gateway::Inbound inbound;
                        inbound.groups = algorithm::transform( regular, transform_inbound);
                        result.inbound = std::move( inbound);
                     }
                        
                  }

                  // outbounds
                  {
                     auto [ reversed, regular] = algorithm::stable::partition( model.outbound.groups, is_reversed);


                     if( regular)
                     {
                        configuration::user::gateway::Outbound outbound;
                        outbound.groups = algorithm::transform( regular, transform_outbound);
                        result.outbound = std::move( outbound);
                     }
                     
                     if( reversed)
                     {
                        configuration::user::gateway::Outbound outbound;
                        outbound.groups = algorithm::transform( reversed, transform_outbound);
                        reverse.outbound = std::move( outbound);
                     }
                  }

                  if( reverse.inbound || reverse.outbound)
                     result.reverse = std::move( reverse);
                  
                  return result;
               }

               auto queue( const configuration::Model& model)
               {
                  Trace trace{ "configuration::model::local::model::queue"};

                  user::queue::Manager result;
                  result.note  = null_if_empty( model.queue.note);

                  result.groups = null_if_empty( algorithm::transform( model.queue.groups, []( auto& value)
                  {
                     user::queue::Group result;
                     result.alias = null_if_empty( value.alias);
                     result.queuebase = null_if_empty( value.queuebase);

                     result.queues = algorithm::transform( value.queues, []( auto& value)
                     {
                        user::queue::Queue result;
                        result.name = value.name;
                        if( ! value.retry.empty())
                        {
                           user::queue::Queue::Retry retry;
                           retry.count = null_if_empty( value.retry.count);
                           retry.delay = null_if_empty( chronology::to::string( value.retry.delay));
                           result.retry = std::move( retry);
                        }
                        
                        result.note  = null_if_empty( value.note);
                        return result;
                     });
                     result.note  = null_if_empty( value.note);
                     return result;
                  }));

                  if( ! model.queue.forward.groups.empty())
                  {
                     user::queue::Forward forward;

                     forward.groups = null_if_empty( algorithm::transform( model.queue.forward.groups, []( auto& group)
                     {
                        user::queue::forward::Group result;
                        result.alias = group.alias;
                        result.note = null_if_empty( group.note);

                        result.services = null_if_empty( algorithm::transform( group.services, []( auto& service)
                        {
                           user::queue::forward::Service result;
                           result.alias = service.alias;
                           result.instances = service.instances;
                           result.note = null_if_empty( service.note);
                           result.source = service.source;
                           result.target.service = service.target.service;

                           if( service.reply)
                           {
                              user::queue::forward::Service::Reply reply;
                              reply.queue = service.reply.value().queue;
                              reply.delay = chronology::to::string( service.reply.value().delay);
                              result.reply = std::move( reply);
                           }

                           return result;
                        }));

                        result.queues = null_if_empty( algorithm::transform( group.queues, []( auto& queue)
                        {
                           user::queue::forward::Queue result;
                           result.alias = queue.alias;
                           result.instances = queue.instances;
                           result.note = null_if_empty( queue.note);
                           result.source = queue.source;
                           result.target.queue = queue.target.queue;
                           result.target.delay = chronology::to::string( queue.target.delay);

                           return result;
                        }));


                        return result;
                     }));
                     result.forward = std::move( forward);
                  }
                  
                  return result;
               }
            } // model

         } // <unnamed>
      } // local

      configuration::Model transform( user::Domain domain)
      {
         Trace trace{ "configuration::model::transform domain"};

         // make sure we normalize alias placeholders and such..
         domain = normalize( std::move( domain));

         configuration::Model result;
         result.domain = local::domain( domain);
         result.service = local::service( domain);
         result.transaction = local::transaction( domain);
         result.gateway = local::gateway( domain);
         result.queue = local::queue( domain);

         log::line( verbose::log, "result: ", result);

         return result;
      }

      user::Domain transform( const configuration::Model& domain)
      {
         Trace trace{ "configuration::model::transform model"};

         auto result = local::model::domain( domain);
         result.services = local::model::service( domain);
         result.transaction = local::model::transaction( domain);
         result.gateway = local::model::gateway( domain.gateway);
         result.queue = local::model::queue( domain);

         return result;
      }

   } // configuration::model
} // casual
