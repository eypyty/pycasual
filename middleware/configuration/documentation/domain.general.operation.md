# configuration domain (general) 

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  


             
## domain.groups

Defines the groups in the configuration. Groups are used to associate `resources` to serveres/executables
and to define the dependency order.

property       | description
---------------|----------------------------------------------------
name           | the name (unique key) of the group.
dependencies   | defines which other groups this group has dependency to.
resources      | defines which resources (name) this group associate with (transient to the members of the group)


## domain.servers

Defines all servers of the configuration (and domain) 

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the server. If not provided basename of `path` will be used
arguments      | arguments to `tpsvrinit` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order, and possible resource association)
restrictions   | regex pattern, if provided only the services that matches at least one of the patterns are actually advertised.
resources      | explicit resource associations (transaction.resources.name)


## domain.executables

Defines all _ordinary_ executables of the configuration. Could be any executable with a `main` function

property       | description
---------------|----------------------------------------------------
path           | the path to the binary, can be relative to `CASUAL_DOMAIN_HOME`, or implicit via `PATH`.
alias          | the logical (unique) name of the executable. If not provided basename of `path` will be used
arguments      | arguments to `main` during startup.
instances      | number of instances to start of the server.
memberships    | which groups are the server member of (dictates order)


## domain.service

Defines _global_ service information that will be used as configuration on services not specifying specific values in the _services_ section.

property                   | description
---------------------------|----------------------------------------------------
execution.timeout.duration | timeout of service, from the _caller_ perspective (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)
execution.timeout.contract | defines action to take if timeout is passed (linger = just wait, kill = send kill signal, terminate = send terminate signal)

## domain.services

Defines service related configuration. 

Note that this configuration is tied to the service, regardless who has advertised the service.

property                   | description
---------------------------|----------------------------------------------------
name                       | name of the service
routes                     | defines what logical names are actually exposed. For _aliases_, it's important to include the original name.
execution.timeout.duration | timeout of service, from the _caller_ perspective (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)
execution.timeout.contract | defines action to take if timeout is passed (linger = just wait, kill = send kill signal, terminate = send terminate signal)

## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
domain:
  name: "domain-name"
  default:
    server:
      instances: 1
      restart: true
    executable:
      instances: 1
      restart: false
  environment:
    variables:
      - key: "SOME_VARIABLE"
        value: "42"
      - key: "SOME_OTHER_VARIABLE"
        value: "some value"
  groups:
    - name: "common-group"
      note: "group that logically groups 'common' stuff"
    - name: "customer-group"
      note: "group that logically groups 'customer' stuff"
      resources:
        - "customer-db"
      dependencies:
        - "common-group"
    - name: "sales-group"
      note: "group that logically groups 'customer' stuff"
      resources:
        - "sales-db"
        - "event-queue"
      dependencies:
        - "customer-group"
  servers:
    - path: "/some/path/customer-server-1"
      memberships:
        - "customer-group"
    - path: "/some/path/customer-server-2"
      memberships:
        - "customer-group"
    - path: "/some/path/sales-server"
      alias: "sales-pre"
      note: "the only services that will be advertised are the ones who matches regex \"preSales.*\""
      instances: 10
      memberships:
        - "sales-group"
      restrictions:
        - "preSales.*"
    - path: "/some/path/sales-server"
      alias: "sales-post"
      note: "he only services that will be advertised are the ones who matches regex \"postSales.*\""
      memberships:
        - "sales-group"
      restrictions:
        - "postSales.*"
    - path: "/some/path/sales-broker"
      memberships:
        - "sales-group"
      environment:
        variables:
          - key: "SALES_BROKER_VARIABLE"
            value: "556"
      resources:
        - "event-queue"
  executables:
    - path: "/some/path/mq-server"
      arguments:
        - "--configuration"
        - "/path/to/configuration"
      memberships:
        - "common-group"
...

````
### json
```` json
{
    "domain": {
        "name": "domain-name",
        "default": {
            "server": {
                "instances": 1,
                "restart": true
            },
            "executable": {
                "instances": 1,
                "restart": false
            }
        },
        "environment": {
            "variables": [
                {
                    "key": "SOME_VARIABLE",
                    "value": "42"
                },
                {
                    "key": "SOME_OTHER_VARIABLE",
                    "value": "some value"
                }
            ]
        },
        "groups": [
            {
                "name": "common-group",
                "note": "group that logically groups 'common' stuff"
            },
            {
                "name": "customer-group",
                "note": "group that logically groups 'customer' stuff",
                "resources": [
                    "customer-db"
                ],
                "dependencies": [
                    "common-group"
                ]
            },
            {
                "name": "sales-group",
                "note": "group that logically groups 'customer' stuff",
                "resources": [
                    "sales-db",
                    "event-queue"
                ],
                "dependencies": [
                    "customer-group"
                ]
            }
        ],
        "servers": [
            {
                "path": "/some/path/customer-server-1",
                "memberships": [
                    "customer-group"
                ]
            },
            {
                "path": "/some/path/customer-server-2",
                "memberships": [
                    "customer-group"
                ]
            },
            {
                "path": "/some/path/sales-server",
                "alias": "sales-pre",
                "note": "the only services that will be advertised are the ones who matches regex \"preSales.*\"",
                "instances": 10,
                "memberships": [
                    "sales-group"
                ],
                "restrictions": [
                    "preSales.*"
                ]
            },
            {
                "path": "/some/path/sales-server",
                "alias": "sales-post",
                "note": "he only services that will be advertised are the ones who matches regex \"postSales.*\"",
                "memberships": [
                    "sales-group"
                ],
                "restrictions": [
                    "postSales.*"
                ]
            },
            {
                "path": "/some/path/sales-broker",
                "memberships": [
                    "sales-group"
                ],
                "environment": {
                    "variables": [
                        {
                            "key": "SALES_BROKER_VARIABLE",
                            "value": "556"
                        }
                    ]
                },
                "resources": [
                    "event-queue"
                ]
            }
        ],
        "executables": [
            {
                "path": "/some/path/mq-server",
                "arguments": [
                    "--configuration",
                    "/path/to/configuration"
                ],
                "memberships": [
                    "common-group"
                ]
            }
        ]
    }
}
````
### ini
```` ini

[domain]
name=domain-name

[domain.default]

[domain.default.executable]
instances=1
restart=false

[domain.default.server]
instances=1
restart=true

[domain.environment]

[domain.environment.variables]
key=SOME_VARIABLE
value=42

[domain.environment.variables]
key=SOME_OTHER_VARIABLE
value=some value

[domain.executables]
arguments=--configuration
arguments=/path/to/configuration
memberships=common-group
path=/some/path/mq-server

[domain.groups]
name=common-group
note=group that logically groups 'common' stuff

[domain.groups]
dependencies=common-group
name=customer-group
note=group that logically groups 'customer' stuff
resources=customer-db

[domain.groups]
dependencies=customer-group
name=sales-group
note=group that logically groups 'customer' stuff
resources=sales-db
resources=event-queue

[domain.servers]
memberships=customer-group
path=/some/path/customer-server-1

[domain.servers]
memberships=customer-group
path=/some/path/customer-server-2

[domain.servers]
alias=sales-pre
instances=10
memberships=sales-group
note=the only services that will be advertised are the ones who matches regex "preSales.*"
path=/some/path/sales-server
restrictions=preSales.*

[domain.servers]
alias=sales-post
memberships=sales-group
note=he only services that will be advertised are the ones who matches regex "postSales.*"
path=/some/path/sales-server
restrictions=postSales.*

[domain.servers]
memberships=sales-group
path=/some/path/sales-broker
resources=event-queue

[domain.servers.environment]

[domain.servers.environment.variables]
key=SALES_BROKER_VARIABLE
value=556

````
### xml
```` xml
<?xml version="1.0"?>
<domain>
 <name>domain-name</name>
 <default>
  <server>
   <instances>1</instances>
   <restart>true</restart>
  </server>
  <executable>
   <instances>1</instances>
   <restart>false</restart>
  </executable>
 </default>
 <environment>
  <variables>
   <element>
    <key>SOME_VARIABLE</key>
    <value>42</value>
   </element>
   <element>
    <key>SOME_OTHER_VARIABLE</key>
    <value>some value</value>
   </element>
  </variables>
 </environment>
 <groups>
  <element>
   <name>common-group</name>
   <note>group that logically groups 'common' stuff</note>
  </element>
  <element>
   <name>customer-group</name>
   <note>group that logically groups 'customer' stuff</note>
   <resources>
    <element>customer-db</element>
   </resources>
   <dependencies>
    <element>common-group</element>
   </dependencies>
  </element>
  <element>
   <name>sales-group</name>
   <note>group that logically groups 'customer' stuff</note>
   <resources>
    <element>sales-db</element>
    <element>event-queue</element>
   </resources>
   <dependencies>
    <element>customer-group</element>
   </dependencies>
  </element>
 </groups>
 <servers>
  <element>
   <path>/some/path/customer-server-1</path>
   <memberships>
    <element>customer-group</element>
   </memberships>
  </element>
  <element>
   <path>/some/path/customer-server-2</path>
   <memberships>
    <element>customer-group</element>
   </memberships>
  </element>
  <element>
   <path>/some/path/sales-server</path>
   <alias>sales-pre</alias>
   <note>the only services that will be advertised are the ones who matches regex "preSales.*"</note>
   <instances>10</instances>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <restrictions>
    <element>preSales.*</element>
   </restrictions>
  </element>
  <element>
   <path>/some/path/sales-server</path>
   <alias>sales-post</alias>
   <note>he only services that will be advertised are the ones who matches regex "postSales.*"</note>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <restrictions>
    <element>postSales.*</element>
   </restrictions>
  </element>
  <element>
   <path>/some/path/sales-broker</path>
   <memberships>
    <element>sales-group</element>
   </memberships>
   <environment>
    <variables>
     <element>
      <key>SALES_BROKER_VARIABLE</key>
      <value>556</value>
     </element>
    </variables>
   </environment>
   <resources>
    <element>event-queue</element>
   </resources>
  </element>
 </servers>
 <executables>
  <element>
   <path>/some/path/mq-server</path>
   <arguments>
    <element>--configuration</element>
    <element>/path/to/configuration</element>
   </arguments>
   <memberships>
    <element>common-group</element>
   </memberships>
  </element>
 </executables>
</domain>

````
