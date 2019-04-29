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

#ifndef __FASTSMRSIMULATION_NODECTRL_H_
#define __FASTSMRSIMULATION_NODECTRL_H_

#include <algorithm>
#include <functional>

#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include "../jsoncpp/json/json.h"

#include "bits/streambuf_iterator.h"
#include "bits/algorithmfwd.h"

#include "../global/UnderlayConfiguratorAccess.h"
#include "../global/GlobalStatisticsAccess.h"
#include "../others/Clock.h"
#include "../common/sdbm.h"
#include "../common/HostBase.h"
#include "../common/Util.h"
#include "../common/MiscWatch.h"
#include "../objects/IPAddress.h"
#include "../objects/IPvXAddress.h"
#include "../objects/SimpleNodeEntry.h"
#include "../objects/SimpleInfo.h"

#include "../others/InterfaceTableAccess.h"

#include "../messages/CycleEvent_m.h"
#include "../messages/RoundQuery_m.h"
#include "../messages/RoundQueryReply_m.h"
#include "../messages/InitJoin_m.h"
#include "../messages/Join_m.h"
#include "../messages/ConsensusQuery_m.h"
#include "../messages/ConsensusNAck_m.h"
#include "../messages/Proposal_m.h"
#include "../messages/Decision_m.h"
#include "../messages/Final_m.h"
#include "../messages/CCQueryReply_m.h"
#include "../messages/CCEnd_m.h"
#include "../messages/GREnd_m.h"
#include "../messages/HBResponse_m.h"
#include "../messages/HBProbe_m.h"
#include "../messages/CCNAck_m.h"
#include "../messages/CCAck_m.h"
#include "../messages/CCQuery_m.h"
#include "../messages/GRQuery_m.h"
#include "../messages/GRQueryReply_m.h"
#include "../messages/GRNAck_m.h"
#include "../messages/Event_m.h"
#include "../messages/Update_m.h"
#include "../messages/Gossip_m.h"
#include "../messages/EventApply_m.h"
#include "../messages/UpdateApply_m.h"
#include "../messages/HBTimeout_m.h"
#include "../messages/ConnectReply_m.h"
#include "../messages/ConfigUpdate_m.h"
#include "../messages/HandShakeReject_m.h"
#include "../messages/HandShake_m.h"
#include "../messages/InitApp_m.h"
#include "../messages/Restore_m.h"
#include "../messages/GRSilent_m.h"

using namespace std;
using namespace omnetpp;
using namespace boost;
using namespace boost::tuples;
using namespace boost::adaptors;

using namespace Json;

class NodeApp;

class NodeCtrl: public HostBase {

private:
    // name of the logical computer
    string LCName;
    /*
     * groupId >=0 for an existing replica,
     * groupId < 0 for a new replica (before joining a group).
     */
    int groupId;
    // here, the value of the clientId actually is the peer_id of the application
    unsigned long clientId;

    /*
     * Network connections
     */
    // Group members, a list of tuple (host full name, out-gate Id to the host) with self included.
    // This map cannot be assigned to WATCH_MAP, which will cause compilation error.
//    map<string, int> neighbors;
    set<string> members;
    // for reconfiguration
//    map<string, int> neighborsTemp;
    set<string> membersTemp;
    // The Survivals list, from Rendezvous, self excluded.
//    map<string, int> survivals;
    set<string> survivals;
    // store the seniority of self and neighbors, controlled by Rendezvous
    map<string, int> seniorities;
    // store the address of clients
//    map<string, IPvXAddress> clients;
    map<string, string> clients;

    // the ipv4 address of the coordinator (i.e., group leader)
    string coord;
    // coordinator candidate
    string coordCand;
    /*
     * epoch >=0 for an existing replica,
     * epoch < 0 for a new replica (before joining a group).
     */
    int epoch;
    /*
     * Temporary groupId for group reconfiguration. gid_temp will be copied to groupId
     * only after the new configuration is loaded.
     */
    int gid_temp;
    // the set of <replica address, snapshot> pairs
    map<string, string> CCSnapshots;
    // the set of <replica address, snapshot> pairs
    map<string, string> GRSnapshots;
    set<string> CCAcks;
    int reconfigTH;

    /*
     * Timers
     */
//    simtime_t applyCycle;
    simtime_t roundCycle;
    simtime_t cycleStart;
    simtime_t gcCycle;
    simtime_t next_round_time;

    /*
     * Rounds
     */
    // for event collection
    // round ID, start from 0
    int round;
    // record the round of events which has been delivered to Qd
    int deliveryRound;
    // record the round of applied events
    int appliedRound;

    /*
     * Message receiving related data structures
     */
//    map<unsigned long, int> NSenders;
    // tuple (client ID, seq_s - r_s) where seq_s - r_s is the difference of start sequence and joining round
    map<unsigned long, int> senders;
    /*
     * JL[s] + 1 is the lowest deliverable sequence of sender s, i.e., MinSeq(s, c) in the original paper,
     * in tuple (sender ID, highest delivered sequence # of non-empty event)
     */
    map<unsigned long, int> JL;
    // record the first round (i.e., start round) of each sender, tuple (sender ID, start round) set of senders
    map<unsigned long, int> Nc;

    /*
     * Message sending related data structure
     */
    // tuple (client ID, seq_s - r_s) where seq_s - r_s is the difference of start sequence and joining round
//    map<unsigned long, int> recipients;
    // tuple (sender ID, start round) recipients
    map<unsigned long, int> Nr;
    /*
     * Update receipt clients / groups, message sending indicators,
     * in tuple (recipient/recipient group ID, recipient(s) address (address, name))
     */
    map<unsigned long, map<string, string> > Rg;

    /*
     * Data structures for event sequence ordering
     */
    // tuple (sender ID, sequence #)
    typedef boost::tuple<unsigned long, int> tuple_qr;
    // event receipt buffer
    map<tuple_qr, string> Qr;
    // tuple (round#, sender ID, sequence number)
    typedef boost::tuple<int, unsigned long, int> tuple_qp;
    // temporary buffer for event collection
    map<tuple_qp, string> Qp;
    // tuple (sender ID, sequence number, event content)
    typedef boost::tuple<unsigned long, int, string> tuple_qc;
    // consensus decision buffer, one round ID mapped onto multiple senders and their events
    multimap<int, tuple_qc> Qc;
    // key of events in Qd, in (round#, client ID, sequence number)
    typedef boost::tuple<int, unsigned long, int> tuple_qd;
    // event delivery queue, in (key, event content)
    map<tuple_qd, string> Qd;

    /*
     * Data structures for event delivery to the application layer
     */
    // tuple (round#, client ID, sequence number))
    boost::tuple<int, unsigned long, int> lastDelivered;
    // the applied state from the application layer, with the key corresponding to the key in Qd
    pair<tuple_qd, unsigned long> state;

    /*
     * Data structure for consensus
     */
    // record the cycles where consensus is to be triggered and the range of event sequences for consensus
    map<int, string> toPropose;
    // record the cycles where consensus has been triggered
    set<int> proposed;
    // store received proposal, in tuple ((round#, host name), {(sender id, sequence #, event)}), for ordering
    typedef boost::tuple<int, string> tuple_pKey;
    typedef boost::tuple<unsigned long, int> tuple_ppKey;
    map<tuple_pKey, map<tuple_ppKey, string> > proposals;

    /*
     * For garbage collection gossip
     */
    // tuple (host, last round, send time)
    typedef boost::tuple<int, simtime_t> tuple_h;
    map<string, tuple_h> histories;

    /*
     * Events
     */
    cMessage* updateTimeout;
    cMessage* ECTimeout;
    cMessage* GCTimeout;
    HBTimeout* hbTimeout;
    // heartbeat timeout check
    bool is_hbto;
    // client connection timeout
    bool is_ccto;
    // start to suspect client
    bool suspect_client;
    simtime_t suspectStart;
    simtime_t client_interruption;

    // number of consensus instances
    int round_query;
    set<int> query_round;

    int STAGE_NUM;

    NodeApp* app;

    /*
     * for result recording
     */
    simsignal_t QdLength;

    void handleHeartbeat(HBProbe* hb);
    void handleHeartbeatTimeout(HBTimeout* hbt);
    void initNode(simtime_t starttime);
    // handle new client incoming
    void handleJoin(InitJoin* initJoin);
    // handle new neighbor incoming
    void handleHandShake(HandShake* hs);

    void stateTransfer();

    /*
     * coordinate reconfiguration protocol
     */
    // true - trigger coordinator change, false - do not trigger coordinator change
    bool checkCoord();
    // compute the coordinator candidate
    string coordinator();
    void handleCCQuery(CCQuery* cc);
    string buildCCSnapshot();
    void handleCCNAck(CCNAck* ccnack);
    void handleCCQueryReply(CCQueryReply* ccr);
    void checkCCSnapshos();
    Value computeCCSnapshot();
    void handleCCEnd(CCEnd* cce);
    void handleCCAck(CCAck* ccack);
    // for uniform agreement on the end of leader election
    void checkCCAcks();

    /*
     * group reconfiguration protocol
     */
    // true - trigger reconfiguration, false - do not trigger reconfiguration
    bool checkConfig();
    string buildRestoreSnapshot();
    void handleGRSilent(GRSilent* gr);
    void handleRestore(Restore* restore);
    void handleGRQuery(GRQuery* gr);
    string buildGRSnapshot();
    void handleGRNAck(GRNAck* grnack);
    void handleGRQueryReply(GRQueryReply* grr);
    void checkGRSnapshots();
    Value computeGRSnapshot();
    void handleGREnd(GREnd* gre);

    /*
     * event handling protocol
     */
    void receiveEvent(Event* event);
    void collectRoundEvents();
    void deliverCycleEvent();
    // handle the (unreliably) broadcast event from a member replica
    void handleEvent(CycleEvent* ce);
    void handleRoundQuery(RoundQuery* query);
    void handleRoundQueryReply(RoundQueryReply* rqr);

    void initConsensus();
    void startPropose(ConsensusQuery* cq);
    void collectProposal(Proposal* proposal);
    void handleConsensusNAck(ConsensusNAck* nack);
    void checkProposals(int r);
    map<boost::tuple<int, unsigned long, int>, string> decide(int r,
            set<string> neighborList);
    void handleDecision(Decision* decision);
    string roundEToString(int r, string senderSeqRanges);
    // apply the first event in Qd by encoding it and then adding it with the second value
    void applyEvent();
    void applyUpdate(UpdateApply* updateApply);
    void disseminateEvent();

    /*
     * garbage collection protocols
     */
    void gossip();
    void handleGossip(Gossip* gossip);

    // return the sent message for faking the send-to-self process
//    virtual void broadcast(cMessage* message, set<string> hosts);
    /**
     * Send message to the UDP module
     */
//    virtual void sendMsg(cMessage* msg);
//    virtual void sendReliable(cMessage* msg);
    /*
     * Return the intersect of survivals and neighbors
     */
    set<string> survivedMembers();
    set<string> survivedList(set<string> membList);

public:
    /*
     * 0 - Event Handling (EH); 1 - Group Reconfiguration (GR); 2 - Coordinator Change (CC);
     * 3 - dormant state (DORMANT, before joining a functional group)
     */
    enum Mode {
        EH, GR, CC, DORMANT
    };
    Mode mode;
    bool receiveClient;

    NodeCtrl();
    virtual ~NodeCtrl();

protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage *msg);
//    virtual simtim_t readClock();
};

#endif
