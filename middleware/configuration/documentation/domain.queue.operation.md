# configuration queue

[//]: # (Attention! this is a generated markdown from casual-configuration-documentation - do not edit this file!)

This is the runtime configuration for a casual domain.

The sections can be splitted in arbitrary ways, and aggregated to one configuration model `casual` uses.
Same sections can be defined in different files, hence, give more or less unlimited configuration setup.

Most of the _sections_ has a property `note` for the user to provide descriptions of the documentation. 
The examples further down uses this to make it easier to understand the examples by them self. But can of
course be used as a mean to help document actual production configuration.  




## domain.queue

Defines the queue configuration

### domain.queue.groups

Defines groups of queues which share the same storage location. Groups has no other meaning.

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the group.
queuebase      | the path to the storage file. (default: `${CASUAL_DOMAIN_HOM}/queue/<group-name>.qb`, if `:memory:` is used, the storage is non persistent)
queues         | defines all queues in this group, see below

#### domain.queue.groups.queues

property       | description
---------------|----------------------------------------------------
name           | the (unique) name of the queue.
retry.count    | number of rollbacks before moving message to `<queue-name>.error`.
retry.delay    | if message is rolled backed, how long delay before message is avaliable for dequeue. (example: `30ms`, `1h`, `3min`, `40s`. if no SI unit `s` is used)


## examples 

Below follows examples in human readable formats that `casual` can handle

### yaml
```` yaml
---
domain:
  queue:
    note: "retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\n"
    default:
      queue:
        retry:
          count: 3
          delay: "20s"
      directory: "${CASUAL_DOMAIN_HOME}/queue/groups"
    groups:
      - alias: "A"
        note: "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb"
        queues:
          - name: "a1"
          - name: "a2"
            retry:
              count: 10
              delay: "100ms"
            note: "after 10 rollbacked dequeues, message is moved to a2.error"
          - name: "a3"
          - name: "a4"
      - alias: "B"
        queuebase: "/some/fast/disk/queue/groupB.qb"
        queues:
          - name: "b1"
          - name: "b2"
            retry:
              count: 20
            note: "after 20 rollbacked dequeues, message is moved to b2.error. retry.delay is 'inherited' from default, if any"
      - alias: "C"
        queuebase: ":memory:"
        note: "group is an in-memory queue, hence no persistence"
        queues:
          - name: "c1"
          - name: "c2"
    forward:
      default:
        service:
          instances: 3
          reply:
            delay: "2s"
        queue:
          instances: 1
          target:
            delay: "500ms"
      groups:
        - alias: "forward-group-1"
          services:
            - source: "b1"
              instances: 4
              target:
                service: "casual/example/echo"
              reply:
                queue: "a3"
                delay: "10ms"
          queues:
            - source: "c1"
              target:
                queue: "a4"
        - alias: "forward-group-2"
          services:
            - source: "b2"
              target:
                service: "casual/example/echo"
...

````
### json
```` json
{
    "domain": {
        "queue": {
            "note": "retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\n",
            "default": {
                "queue": {
                    "retry": {
                        "count": 3,
                        "delay": "20s"
                    }
                },
                "directory": "${CASUAL_DOMAIN_HOME}/queue/groups"
            },
            "groups": [
                {
                    "alias": "A",
                    "note": "will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb",
                    "queues": [
                        {
                            "name": "a1"
                        },
                        {
                            "name": "a2",
                            "retry": {
                                "count": 10,
                                "delay": "100ms"
                            },
                            "note": "after 10 rollbacked dequeues, message is moved to a2.error"
                        },
                        {
                            "name": "a3"
                        },
                        {
                            "name": "a4"
                        }
                    ]
                },
                {
                    "alias": "B",
                    "queuebase": "/some/fast/disk/queue/groupB.qb",
                    "queues": [
                        {
                            "name": "b1"
                        },
                        {
                            "name": "b2",
                            "retry": {
                                "count": 20
                            },
                            "note": "after 20 rollbacked dequeues, message is moved to b2.error. retry.delay is 'inherited' from default, if any"
                        }
                    ]
                },
                {
                    "alias": "C",
                    "queuebase": ":memory:",
                    "note": "group is an in-memory queue, hence no persistence",
                    "queues": [
                        {
                            "name": "c1"
                        },
                        {
                            "name": "c2"
                        }
                    ]
                }
            ],
            "forward": {
                "default": {
                    "service": {
                        "instances": 3,
                        "reply": {
                            "delay": "2s"
                        }
                    },
                    "queue": {
                        "instances": 1,
                        "target": {
                            "delay": "500ms"
                        }
                    }
                },
                "groups": [
                    {
                        "alias": "forward-group-1",
                        "services": [
                            {
                                "source": "b1",
                                "instances": 4,
                                "target": {
                                    "service": "casual/example/echo"
                                },
                                "reply": {
                                    "queue": "a3",
                                    "delay": "10ms"
                                }
                            }
                        ],
                        "queues": [
                            {
                                "source": "c1",
                                "target": {
                                    "queue": "a4"
                                }
                            }
                        ]
                    },
                    {
                        "alias": "forward-group-2",
                        "services": [
                            {
                                "source": "b2",
                                "target": {
                                    "service": "casual/example/echo"
                                }
                            }
                        ]
                    }
                ]
            }
        }
    }
}
````
### ini
```` ini

[domain]

[domain.queue]
note=retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback\n

[domain.queue.default]
directory=${CASUAL_DOMAIN_HOME}/queue/groups

[domain.queue.default.queue]

[domain.queue.default.queue.retry]
count=3
delay=20s

[domain.queue.forward]

[domain.queue.forward.default]

[domain.queue.forward.default.queue]
instances=1

[domain.queue.forward.default.queue.target]
delay=500ms

[domain.queue.forward.default.service]
instances=3

[domain.queue.forward.default.service.reply]
delay=2s

[domain.queue.forward.groups]
alias=forward-group-1

[domain.queue.forward.groups.queues]
source=c1

[domain.queue.forward.groups.queues.target]
queue=a4

[domain.queue.forward.groups.services]
instances=4
source=b1

[domain.queue.forward.groups.services.reply]
delay=10ms
queue=a3

[domain.queue.forward.groups.services.target]
service=casual/example/echo

[domain.queue.forward.groups]
alias=forward-group-2

[domain.queue.forward.groups.services]
source=b2

[domain.queue.forward.groups.services.target]
service=casual/example/echo

[domain.queue.groups]
alias=A
note=will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb

[domain.queue.groups.queues]
name=a1

[domain.queue.groups.queues]
name=a2
note=after 10 rollbacked dequeues, message is moved to a2.error

[domain.queue.groups.queues.retry]
count=10
delay=100ms

[domain.queue.groups.queues]
name=a3

[domain.queue.groups.queues]
name=a4

[domain.queue.groups]
alias=B
queuebase=/some/fast/disk/queue/groupB.qb

[domain.queue.groups.queues]
name=b1

[domain.queue.groups.queues]
name=b2
note=after 20 rollbacked dequeues, message is moved to b2.error. retry.delay is 'inherited' from default, if any

[domain.queue.groups.queues.retry]
count=20

[domain.queue.groups]
alias=C
note=group is an in-memory queue, hence no persistence
queuebase=:memory:

[domain.queue.groups.queues]
name=c1

[domain.queue.groups.queues]
name=c2

````
### xml
```` xml
<?xml version="1.0"?>
<domain>
 <queue>
  <note>retry.count - if number of rollbacks is greater, message is moved to error-queue  retry.delay - the amount of time before the message is available for consumption, after rollback
</note>
  <default>
   <queue>
    <retry>
     <count>3</count>
     <delay>20s</delay>
    </retry>
   </queue>
   <directory>${CASUAL_DOMAIN_HOME}/queue/groups</directory>
  </default>
  <groups>
   <element>
    <alias>A</alias>
    <note>will get default queuebase: ${CASUAL_DOMAIN_HOME}/queue/groupA.gb</note>
    <queues>
     <element>
      <name>a1</name>
     </element>
     <element>
      <name>a2</name>
      <retry>
       <count>10</count>
       <delay>100ms</delay>
      </retry>
      <note>after 10 rollbacked dequeues, message is moved to a2.error</note>
     </element>
     <element>
      <name>a3</name>
     </element>
     <element>
      <name>a4</name>
     </element>
    </queues>
   </element>
   <element>
    <alias>B</alias>
    <queuebase>/some/fast/disk/queue/groupB.qb</queuebase>
    <queues>
     <element>
      <name>b1</name>
     </element>
     <element>
      <name>b2</name>
      <retry>
       <count>20</count>
      </retry>
      <note>after 20 rollbacked dequeues, message is moved to b2.error. retry.delay is 'inherited' from default, if any</note>
     </element>
    </queues>
   </element>
   <element>
    <alias>C</alias>
    <queuebase>:memory:</queuebase>
    <note>group is an in-memory queue, hence no persistence</note>
    <queues>
     <element>
      <name>c1</name>
     </element>
     <element>
      <name>c2</name>
     </element>
    </queues>
   </element>
  </groups>
  <forward>
   <default>
    <service>
     <instances>3</instances>
     <reply>
      <delay>2s</delay>
     </reply>
    </service>
    <queue>
     <instances>1</instances>
     <target>
      <delay>500ms</delay>
     </target>
    </queue>
   </default>
   <groups>
    <element>
     <alias>forward-group-1</alias>
     <services>
      <element>
       <source>b1</source>
       <instances>4</instances>
       <target>
        <service>casual/example/echo</service>
       </target>
       <reply>
        <queue>a3</queue>
        <delay>10ms</delay>
       </reply>
      </element>
     </services>
     <queues>
      <element>
       <source>c1</source>
       <target>
        <queue>a4</queue>
       </target>
      </element>
     </queues>
    </element>
    <element>
     <alias>forward-group-2</alias>
     <services>
      <element>
       <source>b2</source>
       <target>
        <service>casual/example/echo</service>
       </target>
      </element>
     </services>
    </element>
   </groups>
  </forward>
 </queue>
</domain>

````
