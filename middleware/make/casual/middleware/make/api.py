import inspect
import os
import pprint

from casual.make.entity.target import Target, Recipe
import casual.make.entity.recipe as recipe
import casual.make.entity.model as model

import casual.make.api as api
import casual.make.tools.executor as executor
import casual.make.platform.common as common

import importlib
compiler_handler = os.getenv("CASUAL_COMPILER_HANDLER")
selector = importlib.import_module( compiler_handler)

def BUILD_SERVER():
   return "casual-build-server" 

# global setup for operations
link_server_target = model.register( 'link-server')
api.link_target.add_dependency( [link_server_target])

def caller():
   
   name = inspect.getouterframes( inspect.currentframe())[2][1]
   return os.path.abspath( name)

#
# New functions adding functionality
#
def LinkServer( name, objects, libraries, serverdefinition, resources=None, configuration=None):
   """
   Links an XATMI-server
   
   param: name        name of the server with out prefix or suffix.
      
   param: objectfiles    object files that is linked
   
   param: libraries        dependent libraries
   
   param: serverdefinition  path to the server definition file that configure the public services, 
                           and semantics.
                           Can also be a list of public services. I e.  ["service1", "service2"]
                        
   param: resources  optional - a list of XA resources. I e ["db2-rm"] - the names shall 
                  correspond to those defined in $CASUAL_HOME/configuration/resources.(yaml|json|...)
                
   param: configuration optional - path to the resource configuration file
                  this should only be used when building casual it self.
   """

   makefile = caller()
   directory, dummy = os.path.split( makefile)

   full_executable_name = selector.expanded_executable_name(name, directory)
   executable_target = model.register( full_executable_name, full_executable_name, makefile = makefile)

   directive = []
   if resources:
      directive.append( "--resource-keys")
      directive += resources
      
   if configuration:
      directive.append( "--properties-file")
      directive.append( configuration)
   
   if isinstance( serverdefinition, str):
      # We assume it is a path to a server-definition-file
      directive.append( "--server-definition")
      directive.append( serverdefinition)      
   else:
      directive.append( "-s")
      directive += serverdefinition

   arguments = {
      'destination' : executable_target, 
      'objects' : objects, 
      'libraries': libraries, 
      'library_paths': model.library_paths( makefile), 
      'include_paths': model.include_paths( makefile), 
      'directive': directive
      }

   executable_target.add_recipe( Recipe( link_server, arguments))
   executable_target.add_dependency( objects + api.normalize_library_target( libraries))

   link_server_target.add_dependency( model.get( BUILD_SERVER()))
   link_server_target.add_dependency( executable_target)

   api.clean_target.add_recipe( Recipe( recipe.clean, {'filename' : [executable_target], 'makefile': makefile}))

   return executable_target


def link_server( input):
   """
   Recipe for linking objects to casual-servers
   """
   destination = input['destination']
   objects = recipe.make_files( input['objects'])
   context_directory = os.path.dirname( input['destination'].makefile)
   directive = input['directive']
   libraries = input['libraries']
   library_paths = input['library_paths']
   include_paths = input['include_paths']

   BUILDSERVER = [ BUILD_SERVER(), "-c"] + selector.EXECUTABLE_LINKER

   cmd = BUILDSERVER + \
         ['-o', destination.filename] + directive + ['-f'] + [" ".join( objects)] + \
         selector.library_directive(libraries) +  \
         selector.library_paths_directive( library_paths) + \
         common.add_item_to_list( include_paths, '-I')

   executor.execute_command( cmd, destination, context_directory)


