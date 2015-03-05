//!
//! brokervo.h
//!
//! Created on: Sep 30, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_
#define CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"


namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace admin
         {

            struct Group
            {
               struct id_t
               {
                  sf::platform::pid_type pid;
                  sf::platform::queue_id_type queue;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( queue);
                  })

               } id;
               std::string name;
               std::string queuebase;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( queuebase);
               })
            };

            struct Queue
            {
               sf::platform::pid_type group;
               std::size_t id;
               std::string name;
               std::size_t type;
               std::size_t retries;
               std::size_t error;

               struct message_t
               {
                  std::size_t counts = 0;
                  sf::platform::time_type timestamp;

                  struct size_t
                  {
                     std::size_t min = 0;
                     std::size_t max = 0;
                     std::size_t average = 0;
                     std::size_t total = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( min);
                        archive & CASUAL_MAKE_NVP( max);
                        archive & CASUAL_MAKE_NVP( average);
                        archive & CASUAL_MAKE_NVP( total);
                     })
                  } size;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( counts);
                     archive & CASUAL_MAKE_NVP( timestamp);
                     archive & CASUAL_MAKE_NVP( size);
                  })

               } message;



               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( group);
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( retries);
                  archive & CASUAL_MAKE_NVP( error);
                  archive & CASUAL_MAKE_NVP( message);
               })

               friend bool operator < ( const Queue& lhs, const Queue& rhs)
               {
                  return lhs.id < rhs.id;
               }
            };

            struct State
            {
               std::vector< Group> groups;
               std::vector< Queue> queues;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( groups);
                  archive & CASUAL_MAKE_NVP( queues);
               })

            };

            /*
            namespace verbose
            {
               struct GroupVO
               {
                  sf::platform::pid_type pid;
                  sf::platform::queue_id_type queue_id;
                  std::string name;

                  std::vector< QueueVO> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( queues);
                  })

               };
            }
            */


         } // admin
      } // broker
   } // queue

} // casual

#endif // BROKERVO_H_
