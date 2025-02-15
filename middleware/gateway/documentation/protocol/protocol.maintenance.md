
# casual domain protocol _version 1000_

Attention, this documentation refers to **version 1000** (aka, version 1)


Defines what messages are sent between domains and exactly what they contain.

## definitions:

* `fixed array`: an array of fixed size where every element is an 8 bit byte.
* `dynamic array`: an array with dynamic size, where every element is an 8 bit byte.

If an attribute name has `element` in it, for example: `services.element.timeout`, the
data is part of an element in a container. You should read it as `container.element.attribute`

## sizes 

`casual` it self does not impose any size restriction on anything. Whatever the platform supports,`casual` is fine with.
There are however restrictions from the `xatmi` specifcation, regarding service names and such.

`casual` will not apply restriction on sizes unless some specification dictates it, or we got some
sort of proof that a size limitation is needed.

Hence, the maximum sizes below are huge, and it is unlikely that maximum sizes will 
actually work on systems known to `casual` today. But it might in the future... 


## common::communication::message::complete::network::Header 

This header will be the first part of every message below, hence it's name, _Header_

message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

It's probably a good idea (probably the only way) to read the header only, to see how much more you have to read to get
the rest of the message.


role name          | network type   | network size | description                                  
------------------ | -------------- | ------------ | ---------------------------------------------
header.type        | uint64         |            8 | type of the message that the payload contains
header.correlation | (fixed) binary |           16 | correlation id of the message                
header.size        | uint64         |            8 | the size of the payload that follows         

## domain connect messages

Messages that are used to set up a connection

### gateway_domain_connect_request - **#7200**

Connection requests from another domain that wants to connect

role name                 | network type   | network size | description                                            
------------------------- | -------------- | ------------ | -------------------------------------------------------
execution                 | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)     
domain.id                 | (fixed) binary |           16 | uuid of the outbound domain                            
domain.name.size          | uint64         |            8 | size of the outbound domain name                       
domain.name.data          | dynamic string |       [0..*] | dynamic byte array with the outbound domain name       
protocol.versions.size    | uint64         |            8 | number of protocol versions outbound domain can 'speak'
protocol.versions.element | uint64         |            8 | a protocol version                                     
### gateway_domain_connect_reply - **#7201**

Connection reply

role name        | network type   | network size | description                                                       
---------------- | -------------- | ------------ | ------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                
domain.id        | (fixed) binary |           16 | uuid of the inbound domain                                        
domain.name.size | uint64         |            8 | size of the inbound domain name                                   
domain.name.data | dynamic string |       [0..*] | dynamic byte array with the inbound domain name                   
protocol.version | uint64         |            8 | the chosen protocol version to use, or invalid (0) if incompatible

## Discovery messages

### domain discovery 

#### domain_discovery_request - **#7300**

Sent to and received from other domains when one domain wants to discover information abut the other.

role name                     | network type   | network size | description                                                  
----------------------------- | -------------- | ------------ | -------------------------------------------------------------
execution                     | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)           
domain.id                     | (fixed) binary |           16 | uuid of the caller domain                                    
domain.name.size              | uint64         |            8 | size of the caller domain name                               
domain.name.data              | dynamic string |       [0..*] | dynamic byte array with the caller domain name               
content.services.size         | uint64         |            8 | number of requested services to follow (an array of services)
content.services.element.size | uint64         |            8 | size of the current service name                             
content.services.element.data | dynamic string |       [0..*] | dynamic byte array of the current service name               
content.queues.size           | uint64         |            8 | number of requested queues to follow (an array of queues)    
content.queues.element.size   | uint64         |            8 | size of the current queue name                               
content.queues.element.data   | dynamic string |       [0..*] | dynamic byte array of the current queue name                 
#### domain_discovery_reply - **#7301**

Sent to and received from other domains when one domain wants to discover information abut the other.

role name                                 | network type   | network size | description                                                     
----------------------------------------- | -------------- | ------------ | ----------------------------------------------------------------
execution                                 | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)              
domain.id                                 | (fixed) binary |           16 | uuid of the caller domain                                       
domain.name.size                          | uint64         |            8 | size of the caller domain name                                  
domain.name.data                          | dynamic string |       [0..*] | dynamic byte array with the caller domain name                  
content.services.size                     | uint64         |            8 | number of services to follow (an array of services)             
content.services.element.name.size        | uint64         |            8 | size of the current service name                                
content.services.element.name.data        | dynamic string |       [0..*] | dynamic byte array of the current service name                  
content.services.element.category.size    | uint64         |            8 | size of the current service category                            
content.services.element.category.data    | dynamic string |       [0..*] | dynamic byte array of the current service category              
content.services.element.transaction      | uint16         |            2 | service transaction mode (auto, atomic, join, none)             
content.services.element.timeout.duration | uint64         |            8 |                                                                 
content.services.element.hops             | uint64         |            8 | number of domain hops to the service (local services has 0 hops)
content.queues.size                       | uint64         |            8 | number of requested queues to follow (an array of queues)       
content.queues.element.name.size          | uint64         |            8 | size of the current queue name                                  
content.queues.element.name.data          | dynamic string |       [0..*] | dynamic byte array of the current queue name                    
content.queues.element.retries            | uint64         |            8 | how many 'retries' the queue has                                

## Service messages

### Service call 

#### service_call - **#3100**

Sent to and received from other domains when one domain wants call a service in the other domain

role name                | network type   | network size | description                                                        
------------------------ | -------------- | ------------ | -------------------------------------------------------------------
execution                | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
service.name.size        | uint64         |            8 | service name size                                                  
service.name.data        | dynamic string |       [0..*] | byte array with service name                                       
service.timeout.duration | uint64         |            8 | timeout of the service in use (ns)                                 
parent.size              | uint64         |            8 | parent service name size                                           
parent.data              | dynamic string |       [0..*] | byte array with parent service name                                
xid.formatID             | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length         | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length         | uint64         |            8 | length of the transaction branch part                              
xid.data                 | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
flags                    | uint64         |            8 | XATMI flags sent to the service                                    
buffer.type.size         | uint64         |            8 | buffer type name size                                              
buffer.type.data         | dynamic string |       [0..*] | byte array with buffer type in the form 'type/subtype'             
buffer.memory.size       | uint64         |            8 | buffer payload size (could be very big)                            
buffer.memory.data       | dynamic binary |       [0..*] | buffer payload data (with the size of buffer.payload.size)         
#### service_reply - **#3101**

Reply to call request

role name                    | network type   | network size | description                                                                   
---------------------------- | -------------- | ------------ | ------------------------------------------------------------------------------
execution                    | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                            
code.result                  | uint32         |            4 | XATMI result/error code, 0 represent OK                                       
code.user                    | uint64         |            8 | XATMI user supplied code                                                      
transaction.xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported           
transaction.xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                                          
transaction.xid.bqual_length | uint64         |            8 | length of the transaction branch part                                         
transaction.xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)             
transaction.state            | uint8          |            1 | state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY
buffer.type.size             | uint64         |            8 | buffer type name size                                                         
buffer.type.data             | dynamic string |       [0..*] | byte array with buffer type in the form 'type/subtype'                        
buffer.memory.size           | uint64         |            8 | buffer payload size (could be very big)                                       
buffer.memory.data           | dynamic binary |       [0..*] | buffer payload data (with the size of buffer.payload.size)                    

## Transaction messages

### Resource prepare

#### transaction_resource_prepare_request

Sent to and received from other domains when one domain wants to prepare a transaction. 

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64         |            8 | XA flags to be forward to the resource                             
#### transaction_resource_prepare_reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the request          
state            | uint32         |            4 | The state of the operation - If successful XA_OK ( 0)              

### Resource commit

#### transaction_resource_commit_request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64         |            8 | XA flags to be forward to the resource                             
#### transaction_resource_commit_reply

Reply to a commit request. 

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the request          
state            | uint32         |            4 | The state of the operation - If successful XA_OK ( 0)              

### Resource rollback

#### transaction_resource_rollback_request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64         |            8 | XA flags to be forward to the resource                             
#### transaction_resource_rollback_reply

Reply to a rollback request. 

role name        | network type   | network size | description                                                        
---------------- | -------------- | ------------ | -------------------------------------------------------------------
execution        | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
xid.formatID     | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64         |            8 | length of the transaction branch part                              
xid.data         | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource         | uint32         |            4 | RM id of the resource - has to correlate with the request          
state            | uint32         |            4 | The state of the operation - If successful XA_OK ( 0)              

## queue messages

### enqueue 

#### queue_group_enqueue_request - **#6100**

Represent enqueue request.

role name               | network type   | network size | description                                                        
----------------------- | -------------- | ------------ | -------------------------------------------------------------------
execution               | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
name.size               | uint64         |            8 | size of queue name                                                 
name.data               | dynamic string |       [0..*] | data of queue name                                                 
xid.formatID            | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length        | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length        | uint64         |            8 | length of the transaction branch part                              
xid.data                | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
message.id              | (fixed) binary |           16 | id of the message                                                  
message.properties.size | uint64         |            8 | length of message properties                                       
message.properties.data | dynamic string |       [0..*] | data of message properties                                         
message.reply.size      | uint64         |            8 | length of the reply queue                                          
message.reply.data      | dynamic string |       [0..*] | data of reply queue                                                
message.available       | uint64         |            8 | when the message is available for dequeue (us since epoc)          
message.type.size       | uint64         |            8 | length of the type string                                          
message.type.data       | dynamic string |       [0..*] | data of the type string                                            
message.payload.size    | uint64         |            8 | size of the payload                                                
message.payload.data    | dynamic binary |       [0..*] | data of the payload                                                
#### queue_group_enqueue_reply - **#6101**

Represent enqueue reply.

role name | network type   | network size | description                                       
--------- | -------------- | ------------ | --------------------------------------------------
execution | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)
id        | (fixed) binary |           16 | id of the enqueued message                        

### dequeue

#### queue_group_dequeue_request - **#6200**

Represent dequeue request.

role name                | network type   | network size | description                                                        
------------------------ | -------------- | ------------ | -------------------------------------------------------------------
execution                | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
name.size                | uint64         |            8 | size of the queue name                                             
name.data                | dynamic string |       [0..*] | data of the queue name                                             
xid.formatID             | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length         | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length         | uint64         |            8 | length of the transaction branch part                              
xid.data                 | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
selector.properties.size | uint64         |            8 | size of the selector properties (ignored if empty)                 
selector.properties.data | dynamic string |       [0..*] | data of the selector properties (ignored if empty)                 
selector.id              | (fixed) binary |           16 | selector uuid (ignored if 'empty'                                  
block                    | uint8          |            1 | dictates if this is a blocking call or not                         
#### queue_group_dequeue_reply - **#6201**

Represent dequeue reply.

role name                       | network type   | network size | description                                               
------------------------------- | -------------- | ------------ | ----------------------------------------------------------
execution                       | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)        
message.size                    | uint64         |            8 | number of messages dequeued                               
message.element.id              | (fixed) binary |           16 | id of the message                                         
message.element.properties.size | uint64         |            8 | length of message properties                              
message.element.properties.data | dynamic string |       [0..*] | data of message properties                                
message.element.reply.size      | uint64         |            8 | length of the reply queue                                 
message.element.reply.data      | dynamic string |       [0..*] | data of reply queue                                       
message.element.available       | uint64         |            8 | when the message was available for dequeue (us since epoc)
message.element.type.size       | uint64         |            8 | length of the type string                                 
message.element.type.data       | dynamic string |       [0..*] | data of the type string                                   
message.element.payload.size    | uint64         |            8 | size of the payload                                       
message.element.payload.data    | dynamic binary |       [0..*] | data of the payload                                       
message.element.redelivered     | uint64         |            8 | how many times the message has been redelivered           
message.element.timestamp       | uint64         |            8 | when the message was enqueued (us since epoc)             

## conversation messages

### connect 

#### conversation_connect_request - **#3210**

Sent to establish a conversation

role name                | network type   | network size | description                                                        
------------------------ | -------------- | ------------ | -------------------------------------------------------------------
execution                | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)                 
service.name.size        | uint64         |            8 | size of the service name                                           
service.name.data        | dynamic string |       [0..*] | data of the service name                                           
service.timeout.duration | uint64         |            8 | timeout (in ns                                                     
parent.size              | uint64         |            8 | size of the parent service name (the caller)                       
parent.data              | dynamic string |       [0..*] | data of the parent service name (the caller)                       
xid.formatID             | uint64         |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length         | uint64         |            8 | length of the transaction gtrid part                               
xid.bqual_length         | uint64         |            8 | length of the transaction branch part                              
xid.data                 | (fixed) binary |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
duplex                   | uint16         |            2 | in what duplex the callee shall enter (receive:1, send:0)          
buffer.type.size         | uint64         |            8 | buffer type name size                                              
buffer.type.data         | dynamic string |       [0..*] | byte array with buffer type in the form 'type/subtype'             
buffer.memory.size       | uint64         |            8 | buffer payload size (could be very big)                            
buffer.memory.data       | dynamic binary |       [0..*] | buffer payload data (with the size of buffer.payload.size)         
#### conversation_connect_reply - **#3211**

Reply for a conversation

role name   | network type   | network size | description                                       
----------- | -------------- | ------------ | --------------------------------------------------
execution   | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)
code.result | uint32         |            4 | result code of the connection attempt             

### send
#### conversation_send - **#3212**

Represent a message sent 'over' an established connection

role name          | network type   | network size | description                                               
------------------ | -------------- | ------------ | ----------------------------------------------------------
execution          | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)        
duplex             | uint16         |            2 | in what duplex the callee shall enter (receive:1, send:0) 
code.result        | uint32         |            4 | status of the connection                                  
code.user          | uint64         |            8 | user code, if callee did a tpreturn and supplied user-code
buffer.type.size   | uint64         |            8 | buffer type name size                                     
buffer.type.data   | dynamic string |       [0..*] | byte array with buffer type in the form 'type/subtype'    
buffer.memory.size | uint64         |            8 | buffer payload size (could be very big)                   
buffer.memory.data | dynamic binary |       [0..*] | buffer payload data (with the size of buffer.payload.size)

### disconnect

#### conversation_disconnect - **#3213**

Sent to abruptly disconnect the conversation

role name | network type   | network size | description                                       
--------- | -------------- | ------------ | --------------------------------------------------
execution | (fixed) binary |           16 | uuid of the current execution context (breadcrumb)
