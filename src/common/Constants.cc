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

#include "Constants.h"

const char* DISPLAY_STR_CH_TEMP = "m=auto;ls=red,3,dashed";
const char* DISPLAY_STR_CH_HIDDEN = "ls=,0";
const char* DISPLAY_STR_CH_LINK = "m=auto;ls=black,1,solid";

const char* DISPLAY_STR_MSG_GROUP = "b=10,10,oval,black,kind,0";
const char* DISPLAY_STR_MSG_GROUP_CC = "b=10,10,oval,blue,kind,0";
const char* DISPLAY_STR_MSG_GROUP_GR = "b=10,10,oval,yellow,kind,0";
const char* DISPLAY_STR_MSG_GROUP_EH = "b=10,10,oval,cyan,kind,0";
const char* DISPLAY_STR_MSG_HIDDEN = "b=-1,-1,,green,kind,3";

const char* DISPLAY_STR_MOD_HIDDEN = "i=block/downarrow";

const char* CH_TYPE_FAIR_LOSS = "common.FairLossChannel";

const char* event::MISS_EVENT = "0x22A5";
const char* event::EMPTY_EVENT = "Empty";
const char* event::HANDSHAKE = "HANDSHAKE";
const char* event::HANDSHAKE_REPLY = "HANDSHAKE_REPLY";
const char* event::NO_INPUT = "NO_INPUT";
const char* event::TO_JOIN = "TO_JOIN";
const char* event::TO_REMOVE = "TO_REMOVE";
const char* event::GET_BLOCK = "GET_BLOCK";
const char* event::TO_EXIT = "TO_EXIT";
const char* event::TO_INIT = "TO_INIT";

// consensus service
const char* msg::COORD_CHANGE = "COORD_CHANGE";
const char* msg::CONSENSUS_NACK = "CONSENSUS_NACK";
const char* msg::CC_NACK = "CC_NACK";
const char* msg::CC_ACK = "CC_ACK";
const char* msg::GR_NACK = "GR_NACK";
const char* msg::COORD_CHANGE_REPLY = "COORD_CHANGE_REPLY";
const char* msg::LOAD_COORD = "LOAD_COORD";
const char* msg::GROUP_RECONFIG = "GROUP_RECONFIG";
const char* msg::GROUP_RECONFIG_SILENT = "GROUP_RECONFIG_SILENT";
const char* msg::RESTORE = "RESTORE";
const char* msg::RECONFIG_READY = "RECONFIG_READY";
const char* msg::LOAD_CONFIG = "LOAD_CONFIG";
const char* msg::PROPOSAL = "PROPOSAL";
const char* msg::DECISION = "DECISION";
const char* msg::AGREEMENT = "AGREEMENT";
const char* msg::FINAL = "FINAL";
const char* msg::CYCLE_EVENT = "CYCLE_EVENT";
const char* msg::ROUND_QUERY = "ROUND_QUERY";
const char* msg::ROUND_QUERY_REPLY = "ROUND_QUERY_REPLY";
const char* msg::CONSENSUS_QUERY = "CONSENSUS_QUERY";
const char* msg::GOSSIP_GC = "GOSSIP_GC";
const char* msg::EVENT_APPLY = "EVENT_APPLY";
const char* msg::UPDATE_APPLY = "UPDATE_APPLY";
const char* msg::APP_STATE_INIT = "APP_STATE_INIT";
const char* msg::INIT_APP = "INIT_APP";
const char* msg::TERMINATION = "TERMINATION";
const char* msg::REVOKE_LC = "REVOKE_LC";
const char* msg::REVOKE_CLIENT = "REVOKE_CLIENT";
const char* msg::REVOKE_HOST = "REVOKE_HOST";

// messages between client and hosts
const char* msg::EVENT = "EVENT";
const char* msg::UPDATE = "UPDATE";
const char* msg::INIT_JOIN = "INIT_JOIN";
const char* msg::JOIN = "JOIN";
const char* msg::REQ_CONNECT = "REQ_CONNECT";
const char* msg::REQ_CONNECT_NEIGHBOR = "REQ_CONNECT_NEIGHBOR";
const char* msg::REPLY_CONNECT = "REPLY_CONNECT";
const char* msg::REPLY_CONNECT_NEIGHBOR = "REPLY_CONNECT_NEIGHBOR";
const char* msg::CONFIG_UPDATE = "CONFIG_UPDATE";
const char* msg::HANDSHAKE_TIMEOUT = "HANDSHAKE_TIMEOUT";
const char* msg::HANDSHAKE_REJECT = "HANDSHAKE_REJECT";
const char* msg::HANDSHAKE = "HANDSHAKE";

const char* msg::NEIGHBOR_ADD = "NEIGHBOR_ADD";
const char* msg::NEIGHBOR_RM = "NEIGHBOR_RM";

const char* msg::RT_EVENT = "RT_EVENT";

// initialization
const char* msg::HEARTBEAT_INIT = "HEARTBEAT_INIT";
const char* msg::PURGER_INIT = "PURGER_INIT";
const char* msg::LC_INIT = "LC_INIT";

// timeout event
const char* msg::HEARTBEAT_TIMEOUT = "HEARTBEAT_TIMEOUT";
const char* msg::EVENT_CYCLE = "EVENT_CYCLE";
const char* msg::LOAD_TIMEOUT = "LOAD_TIMEOUT";
const char* msg::CONNECT_TIMEOUT = "CONNECT_TIMEOUT";
const char* msg::FAILURE = "FAILURE";
const char* msg::UPDATE_CYCLE = "UPDATE_CYCLE";
const char* msg::NOTIFICATION = "NOTIFICATION";
const char* msg::CHURN_SCAN_CYCLE = "CHURN_SCAN_CYCLE";
const char* msg::EC_START = "EC_START";
const char* msg::INIT_JOIN_TIMEOUT = "INIT_JOIN_TIMEOUT";
const char* msg::EVENT_TIMEOUT = "EVENT_TIMEOUT";
const char* msg::GC_TIMEOUT = "GC_TIMEOUT";
const char* msg::CLIENT_INIT = "CLIENT_INIT";
const char* msg::NEIGHBOR_DISCOVERY = "NEIGHBOR_DISCOVERY";
//const char* msg::STEP_APP = "STEP_APP";

// messages between hosts and Rendezvous
const char* msg::HEARTBEAT = "HEARTBEAT";
const char* msg::HEARTBEAT_RESPONSE = "HEARTBEAT_RESPONSE";

//const char* msg::STEP_CLIENT = "STEP_CLIENT";
const char* msg::APP_DATA = "APP_DATA";
const char* msg::CLIENT_DATA = "CLIENT_DATA";
const char* msg::CS_COMM_LOOP = "CS_COMM_LOOP";

const char* msg::LOGIN_LOOP = "LOGIN_LOOP";

const char* msg::CHORD_LOOK_UP = "CHORD_LOOK_UP";
const char* msg::CHORD_FINAL = "CHORD_FINAL";
const char* msg::CHORD_SUCCESSOR = "CHORD_SUCCESSOR";
const char* msg::CHORD_QUERY_SUCCESSOR = "CHORD_QUERY_SUCCESSOR";
const char* msg::CHORD_SUCCESSOR_FOUND = "CHORD_SUCCESSOR_FOUND";
const char* msg::CHORD_NOTIFY = "CHORD_NOTIFY";
const char* msg::CHORD_STORE = "CHORD_STORE";
const char* msg::CHORD_GET = "CHORD_GET";
const char* msg::CHORD_REPLICATE = "CHORD_REPLICATE";
const char* msg::CHORD_FIX_REPLICA = "CHORD_FIX_REPLICA";
const char* msg::CHORD_UPDATE_PREDECESSOR = "CHORD_UPDATE_PREDECESSOR";
const char* msg::CHORD_MAINT = "CHORD_MAINT";
const char* msg::CHORD_REPLY = "CHORD_REPLY";
const char* msg::INIT_CHORD = "INIT_CHORD";
const char* msg::LABEL_CONTENT = "LABEL_CONTENT";

// number of initialization stage
const int stage::CLIENT_INIT = 2;
const int stage::NODE_CTRL_INIT = 2;
const int stage::NODE_APP_INIT = 2;
const int stage::RENDEZVOUS_INIT = 2;

const char* data::DATA_EMPTY = "DATA NOT FOUND";
