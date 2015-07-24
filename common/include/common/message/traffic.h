//!
//! monitor.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMMONMESSAGEMONITOR_H_
#define COMMMONMESSAGEMONITOR_H_

#include "common/message/type.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace traffic
         {
            namespace monitor
            {
               namespace connect
               {
                  //!
                  //! Used to advertise the monitorserver
                  //!
                  using Reqeust = server::connect::basic_request< cTrafficMonitorConnectRequest>;
                  using Reply = server::connect::basic_reply< cTrafficMonitorConnectReply>;

               } // connect


               //!
               //! Used to unadvertise the monitorserver
               //!
               typedef server::basic_disconnect< cTrafficMonitorDisconnect> Disconnect;


            } // monitor

            //!
            //! Notify traffic-monitor with statistic-event
            //!
            struct Event : basic_message< cTrafficEvent>
            {
               std::string service;
               std::string parent;
               common::process::Handle process;

               common::transaction::ID trid;

               common::platform::time_point start;
               common::platform::time_point end;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  base_type::marshal( archive);
                  archive & service;
                  archive & parent;
                  archive & process;
                  archive & execution;
                  archive & trid;
                  archive & start;
                  archive & end;
               )

               friend std::ostream& operator << ( std::ostream& out, const Event& value);
            };

         } // traffic
      } // message
   } // common
} // casual

#endif // MONITOR_H_
