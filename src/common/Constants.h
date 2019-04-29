//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Display channel strings
extern const char* DISPLAY_STR_CH_TEMP;
extern const char* DISPLAY_STR_CH_HIDDEN;
extern const char* DISPLAY_STR_CH_LINK;

// Display message strings
extern const char* DISPLAY_STR_MSG_GROUP;
extern const char* DISPLAY_STR_MSG_GROUP_CC;
extern const char* DISPLAY_STR_MSG_GROUP_GR;
extern const char* DISPLAY_STR_MSG_GROUP_EH;
extern const char* DISPLAY_STR_MSG_HIDDEN;

// Display module strings
extern const char* DISPLAY_STR_MOD_HIDDEN;

// Channel type strings
extern const char* CH_TYPE_FAIR_LOSS;

// special application-level events
namespace event {
// special string for missing event place-holder
extern const char* MISS_EVENT;
extern const char* EMPTY_EVENT;
extern const char* HANDSHAKE;
extern const char* HANDSHAKE_REPLY;
extern const char* NO_INPUT;
extern const char* TO_JOIN;
extern const char* TO_REMOVE;
extern const char* GET_BLOCK;
extern const char* TO_EXIT;
extern const char* TO_INIT;
}

// message names
namespace msg {
// main group communication protocols
extern const char* COORD_CHANGE;
extern const char* CC_NACK;
extern const char* CC_ACK;
extern const char* GR_NACK;
extern const char* CONSENSUS_NACK;
extern const char* COORD_CHANGE_REPLY;
extern const char* LOAD_COORD;
extern const char* GROUP_RECONFIG;
extern const char* GROUP_RECONFIG_SILENT;
extern const char* RESTORE;
extern const char* RECONFIG_READY;
extern const char* LOAD_CONFIG;
extern const char* PROPOSAL;
extern const char* DECISION;
extern const char* AGREEMENT;
extern const char* FINAL;
extern const char* CYCLE_EVENT;
extern const char* ROUND_QUERY;
extern const char* ROUND_QUERY_REPLY;
extern const char* CONSENSUS_QUERY;
extern const char* GOSSIP_GC;
extern const char* EVENT_APPLY;
extern const char* UPDATE_APPLY;
extern const char* APP_STATE_INIT;

// messages between client and hosts
extern const char* EVENT;
extern const char* UPDATE;
extern const char* INIT_JOIN;
extern const char* JOIN;
extern const char* REQ_CONNECT;
extern const char* REQ_CONNECT_NEIGHBOR;
extern const char* REPLY_CONNECT;
extern const char* REPLY_CONNECT_NEIGHBOR;
extern const char* CONFIG_UPDATE;
extern const char* HANDSHAKE_TIMEOUT;
extern const char* HANDSHAKE_REJECT;
extern const char* HANDSHAKE;
extern const char* CLIENT_TIMEOUT;
extern const char* CLIENT_HB;
extern const char* CLIENT_HB_RESPONSE;

// messages for add/remove neighbor
extern const char* NEIGHBOR_ADD;
extern const char* NEIGHBOR_RM;

// real time telnet message
extern const char* RT_EVENT;

// initialization
extern const char* HEARTBEAT_INIT;
extern const char* PURGER_INIT;
extern const char* LC_INIT;

// termination
extern const char* INIT_APP;
extern const char* TERMINATION;
extern const char* REVOKE_LC;
extern const char* REVOKE_CLIENT;
extern const char* REVOKE_HOST;

// timeout event
extern const char* HEARTBEAT_TIMEOUT;
extern const char* EVENT_CYCLE;
extern const char* LOAD_TIMEOUT;
extern const char* CONNECT_TIMEOUT;
extern const char* FAILURE;
extern const char* UPDATE_CYCLE;
extern const char* NOTIFICATION;
extern const char* CHURN_SCAN_CYCLE;
// start of event collection
extern const char* EC_START;
extern const char* INIT_JOIN_TIMEOUT;
extern const char* EVENT_TIMEOUT;
extern const char* GC_TIMEOUT;
extern const char* CLIENT_INIT;
extern const char* NPC_INIT;
extern const char* NEIGHBOR_DISCOVERY;
//extern const char* STEP_APP;

// messages between hosts and Rendezvous
extern const char* HEARTBEAT;
extern const char* HEARTBEAT_RESPONSE;

// client application
extern const char* STEP_CLIENT;
extern const char* APP_DATA;
extern const char* CLIENT_DATA;
extern const char* CS_COMM_LOOP;

// for client login to Virtual Net
extern const char* LOGIN_LOOP;

// Chord message types
extern const char* CHORD_LOOK_UP;
extern const char* CHORD_FINAL;
extern const char* CHORD_SUCCESSOR;
extern const char* CHORD_QUERY_SUCCESSOR;
extern const char* CHORD_SUCCESSOR_FOUND;
extern const char* CHORD_NOTIFY;
extern const char* CHORD_STORE;
extern const char* CHORD_GET;
extern const char* CHORD_REPLICATE;
extern const char* CHORD_FIX_REPLICA;
extern const char* CHORD_UPDATE_PREDECESSOR;
extern const char* CHORD_MAINT;
extern const char* CHORD_REPLY;
extern const char* INIT_CHORD;
// temporarily not used
extern const char* LABEL_CONTENT;
}

namespace stage {
extern const int CLIENT_INIT;
extern const int NODE_CTRL_INIT;
extern const int NODE_APP_INIT;
extern const int RENDEZVOUS_INIT;
}

namespace data {
// chord data type
extern const char* DATA_EMPTY;
}
#endif /* CONSTANTS_H_ */
