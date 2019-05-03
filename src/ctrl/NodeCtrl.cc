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

#include <iostream>
#include <fstream>
#include "NodeCtrl.h"
#include "../app/NodeApp.h"

std::ofstream dfile("VirtualNet_MeshCtrl.txt");
std::ostream dout(dfile.rdbuf());

Define_Module(NodeCtrl);

NodeCtrl::NodeCtrl() {
    lastDelivered = boost::tuple<int, unsigned long, int>(-1, 0, 0);
    // to make sure the first round is round 0
    round = -1;
    deliveryRound = 0;
    round_query = 0;
    STAGE_NUM = stage::NODE_CTRL_INIT;
    cycleStart = -1;

    clientId = 0;
    receiveClient = true;
    is_hbto = false;
    is_ccto = false;
    suspect_client = false;
    suspectStart = 0;
    reconfigTH = 0;
    lastHB = NULL;

    updateTimeout = new cMessage(msg::UPDATE_CYCLE);
    ECTimeout = new cMessage(msg::EC_START);
    GCTimeout = new cMessage(msg::GC_TIMEOUT);
    hbTimeout = new HBTimeout(msg::HEARTBEAT_TIMEOUT);
}

NodeCtrl::~NodeCtrl() {
    if (updateTimeout != NULL) {
        cancelAndDelete(updateTimeout);
    }
    if (ECTimeout != NULL) {
        cancelAndDelete(ECTimeout);
    }
    if (GCTimeout != NULL) {
        cancelAndDelete(GCTimeout);
    }
    if (hbTimeout != NULL) {
        cancelAndDelete(hbTimeout);
    }
    if (lastHB != NULL) {
        delete lastHB;
    }
}

int NodeCtrl::numInitStages() const {
    return STAGE_NUM;
}

void NodeCtrl::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();

        roundCycle = par("roundCycle");
        gcCycle = par("GCCycle");
        client_interruption = getParentModule()->par("ClientInterruption");

        WATCH(coord);
        WATCH(mode);
        WATCH(epoch);
        WATCH(groupId);
        WATCH(gid_temp);
        WATCH_SET(members);
        WATCH_SET(membersTemp);
        WATCH_SET(survivals);
        WATCH_MAP(seniorities);
        WATCH_MAP(clients);
        WATCH_MAP(toPropose);
        WATCH_SET(proposed);
        WATCH_MAP(senders);
        WATCH_MAP(Nc);
        WATCH_MAP(JL);
        WATCH_MAP(Nr);
        WATCH(round);
        WATCH(deliveryRound);
        WATCH(appliedRound);

        WATCH_MAP(Qr);
        WATCH_MAP(Qd);
        WATCH_MAP(Qp);
        WATCH_MULTIMAP(Qc);
        WATCH_MAP(histories);
        WATCH(lastDelivered);
        WATCH(state.first);
        WATCH(state.second);
        WATCH(round_query);
        WATCH_SET(query_round);
        WATCH(next_round_time);
        WATCH(LCName);
        WATCH(clientId);
        WATCH(is_hbto);
        WATCH(is_ccto);
    }

    if (stage == 1) {
        // init host neighbors
        groupId = gid_temp = -1;
        // initial stage to avoid starting from EH
        mode = DORMANT;
        epoch = -1;
        ipAddress = loadIPAddress(getParentModule()->par("IPInterface"),
                getParentModule());
        getParentModule()->getDisplayString().setTagArg("t", 0,
                ipAddress.get4().str().c_str());

        QdLength = registerSignal("QdLength");

        app = dynamic_cast<NodeApp*>(this->getParentModule()->getSubmodule(
                "app"));

        simtime_t cycle = getParentModule()->par("HBcycle");
        scheduleAt(simTime() + cycle, hbTimeout);
    }
}

void NodeCtrl::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::HEARTBEAT)) {
        HBProbe* hb = check_and_cast<HBProbe*>(msg);
        handleHeartbeat(hb);
        // stage transfer
        stateTransfer();
        // check whether there is any event in Qp
        deliverCycleEvent();
    } else if (msg->isName(msg::COORD_CHANGE)) {
        CCQuery* cc = check_and_cast<CCQuery*>(msg);
        handleCCQuery(cc);
    } else if (msg->isName(msg::CC_NACK)) {
        CCNAck* ccnack = check_and_cast<CCNAck*>(msg);
        handleCCNAck(ccnack);
    } else if (msg->isName(msg::COORD_CHANGE_REPLY)) {
        CCQueryReply* ccr = check_and_cast<CCQueryReply*>(msg);
        handleCCQueryReply(ccr);
    } else if (msg->isName(msg::LOAD_COORD)) {
        CCEnd* cce = check_and_cast<CCEnd*>(msg);
        bool reconfig = cce->getReconfig();
        handleCCEnd(cce);
    } else if (msg->isName(msg::CC_ACK)) {
        CCAck* ack = check_and_cast<CCAck*>(msg);
        handleCCAck(ack);
    } else if (msg->isName(msg::GROUP_RECONFIG)) {
        GRQuery* gr = check_and_cast<GRQuery*>(msg);
        handleGRQuery(gr);
    } else if (msg->isName(msg::GR_NACK)) {
        GRNAck* grnack = check_and_cast<GRNAck*>(msg);
        handleGRNAck(grnack);
    } else if (msg->isName(msg::RECONFIG_READY)) {
        GRQueryReply* grr = check_and_cast<GRQueryReply*>(msg);
        handleGRQueryReply(grr);
    } else if (msg->isName(msg::LOAD_CONFIG)) {
        GREnd* gre = check_and_cast<GREnd*>(msg);
        handleGREnd(gre);
    } else if (msg->isName(msg::EVENT)) {
        Event* event = check_and_cast<Event*>(msg);
        receiveEvent(event);
    } else if (msg->isName(msg::CONSENSUS_QUERY)) {
        ConsensusQuery * cq = check_and_cast<ConsensusQuery*>(msg);
        this->startPropose(cq);
    } else if (msg->isName(msg::CONSENSUS_NACK)) {
        ConsensusNAck * nack = check_and_cast<ConsensusNAck*>(msg);
        this->handleConsensusNAck(nack);
    } else if (msg->isName(msg::PROPOSAL)) {
        Proposal* proposal = check_and_cast<Proposal*>(msg);
        collectProposal(proposal);
    } else if (msg->isName(msg::DECISION)) {
        Decision* decision = check_and_cast<Decision*>(msg);
        handleDecision(decision);
    } else if (msg->isName(msg::UPDATE_CYCLE)) {
        disseminateEvent();
    } else if (msg->isName(msg::EC_START)) {
        collectRoundEvents();
    } else if (msg->isName(msg::CYCLE_EVENT)) {
        if (receiveClient) {
            CycleEvent* ce = check_and_cast<CycleEvent*>(msg);
            handleEvent(ce);
        }
    } else if (msg->isName(msg::ROUND_QUERY)) {
        RoundQuery* query = check_and_cast<RoundQuery*>(msg);
        handleRoundQuery(query);
    } else if (msg->isName(msg::ROUND_QUERY_REPLY)) {
        RoundQueryReply* rqr = check_and_cast<RoundQueryReply*>(msg);
        handleRoundQueryReply(rqr);
    } else if (msg->isName(msg::INIT_JOIN)) {
        if (receiveClient) {
            InitJoin* initJoin = check_and_cast<InitJoin*>(msg);
            handleJoin(initJoin);
        } else {
            delete msg;
        }
    } else if (msg->isName(msg::GC_TIMEOUT)) {
        gossip();
    } else if (msg->isName(msg::GOSSIP_GC)) {
        Gossip* gossip = check_and_cast<Gossip*>(msg);
        handleGossip(gossip);
    } else if (msg->isName(msg::UPDATE_APPLY)) {
        UpdateApply* updateApply = check_and_cast<UpdateApply*>(msg);
        applyUpdate(updateApply);
    } else if (msg->isName(msg::HEARTBEAT_TIMEOUT)) {
        HBTimeout* hbt = check_and_cast<HBTimeout*>(msg);
        handleHeartbeatTimeout(hbt);
    } else if (msg->isName(msg::HANDSHAKE)) {
        if (receiveClient) {
            HandShake* hs = check_and_cast<HandShake*>(msg);
            handleHandShake(hs);
        } else {
            delete msg;
        }
    } else if (msg->isName(msg::GROUP_RECONFIG_SILENT)) {
        GRSilent* gr = check_and_cast<GRSilent*>(msg);
        handleGRSilent(gr);
    } else if (msg->isName(msg::RESTORE)) {
        Restore* restore = check_and_cast<Restore*>(msg);
        handleRestore(restore);
    } else if (msg->isName(msg::CHORD_LOOK_UP)) {
        sendReliable(msg);
    } else if (msg->isName(msg::CHORD_REPLY)) {
        send(msg, this->gateHalf("uplink", cGate::OUTPUT));
    } else if (msg->isName(msg::TERMINATION)) {
        toExit = true;
        if (is_hbto) {

            cout << "heartbeat timeout, start to revoke the node" << endl;

            UnderlayConfiguratorAccess().get()->revokeNode(ipAddress, true);

            delete msg;
        } else {
            sendReliable(msg);
        }
    }
}

void NodeCtrl::handleHeartbeat(HBProbe *hb) {

//    dout << simTime() << " heartbeat arrives" << endl;

    UDPControlInfo *hbCtrl = check_and_cast<UDPControlInfo *>(
            hb->getControlInfo());
    IPvXAddress& srcAddr = hbCtrl->getSrcAddr();
    if (LCName.empty()) {
        IPvXAddress rendezvous = srcAddr;
        addIPAddress(getParentModule()->par("RendezvousInterface"), rendezvous,
                getParentModule());
        LCName = hb->getLCName();
    }
    string dest = hb->getSourceName();
    bool isInit = hb->getIsInit();
    reconfigTH = hb->getThreshold();
    // change survivals list
    string content = hb->getSurvivals();
    vector<string> hostAddrs;
    util::splitString(content, ";", hostAddrs);
    vector<string> toRemove;
    vector<string> toAdd;
    for (set<string>::iterator it = survivals.begin(); it != survivals.end();
            it++) {
        string host = *it;
        // apply the vector search algorithm
        if (find(hostAddrs.begin(), hostAddrs.end(), host) == hostAddrs.end()) {
            toRemove.push_back(host);
        }
    }
    for (size_t i = 0; i < hostAddrs.size(); i++) {
        if (survivals.count(hostAddrs[i]) == 0) {
            toAdd.push_back(hostAddrs[i]);
        }
    }
    // remove failed hosts
    for (size_t i = 0; i < toRemove.size(); i++) {
        survivals.erase(toRemove[i]);
    }
    // add new hosts
    for (size_t i = 0; i < toAdd.size(); i++) {
        string hostAddr = toAdd[i];
        survivals.insert(hostAddr);
    }
    hostAddrs.clear();
    toRemove.clear();
    toAdd.clear();

    if (isInit) {
        simtime_t starttime = hb->getStartTime();
        initNode(starttime);
    }

// record the new heartbeat response
    if (lastHB != NULL) {
        // delete the last received heartbeat
        delete lastHB;
    }

    // send heartbeat response
    HBResponse* hbr = new HBResponse(msg::HEARTBEAT_RESPONSE);
    hbr->setSourceName(fullName);
    hbr->setDestName(dest.c_str());
    // TODO may add clock error later
    hbr->setTimestamp(simTime());
    UDPControlInfo* udpControlInfo = new UDPControlInfo();
    udpControlInfo->setDestAddr(srcAddr);
    udpControlInfo->setSrcAddr(ipAddress);
    hbr->setControlInfo(udpControlInfo);
    sendFixedDelay(hbr);

    hb->setTimestamp(simTime());
    lastHB = hb;
}

void NodeCtrl::handleHeartbeatTimeout(HBTimeout* hbt) {
    simtime_t cycle = getParentModule()->par("HBcycle");
    simtime_t receiveTime = 0;
    if (lastHB != NULL) {
        receiveTime = lastHB->getTimestamp();
    }
    simtime_t diff = simTime() - receiveTime;
    if (diff > cycle) {
        receiveClient = false;
        app->onExit();
        is_hbto = true;

        cout << "heartbeat timeout" << endl;

    } else {
        if (lastHB != NULL) {
            // set heartbeat timeout
            scheduleAt(simTime() + cycle, hbTimeout);
        }
    }
}

void NodeCtrl::initNode(simtime_t starttime) {

//    dout << simTime() << " initNode" << endl;

// group ID starts from 0
    groupId = gid_temp = 0;
    members.insert(survivals.begin(), survivals.end());
    for (set<string>::iterator it = members.begin(); it != members.end();
            it++) {
        seniorities[*it] = 1;
    }
    cycleStart = starttime;
    simtime_t current = simTime();

    round = (current >= starttime) ?
            floor((current - starttime) / roundCycle) : -1;

    cout << "[" << simTime() << "s] " << fullName << " init round: " << round
            << endl;

    next_round_time = starttime + (round + 1) * roundCycle;
    scheduleAt(next_round_time, ECTimeout);
//    scheduleAt(next_round_time, updateTimeout);
    scheduleAt(next_round_time + gcCycle, GCTimeout);
}

void NodeCtrl::handleJoin(InitJoin* initJoin) {
    UDPControlInfo* initJoinCtrl = check_and_cast<UDPControlInfo *>(
            initJoin->getControlInfo());
    IPvXAddress srcAddr = initJoinCtrl->getSrcAddr();
    string addrStr = srcAddr.get4().str();
    if (cycleStart < 0 || LCName.empty()) {
        ConnectReply* reply = new ConnectReply(msg::REPLY_CONNECT);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        reply->setControlInfo(udpControlInfo);
        reply->setLCName(initJoin->getLCName());
        sendFixedDelay(reply);

        dout << simTime() << " cycleStart < 0" << endl;
    } else {
        int seq_s = initJoin->getSeq();
        unsigned long senderId = initJoin->getSenderId();

        dout << simTime() << " " << fullName << " LCName: " << LCName << endl;
        dout << fullName << " sent LCName: " << initJoin->getLCName() << endl;
        if (strcmp(initJoin->getLCName(), LCName.c_str()) == 0) {
            this->clientId = initJoin->getSenderId();
            dout << fullName << " clientId: " << this->clientId
                    << " after update" << endl;
        } else {
            dout << fullName << " clientId: " << this->clientId << endl;
        }

        string senderName = initJoin->getSourceName();
        simtime_t sendTime = initJoin->getJoinTime();
        if (clients.count(senderName) == 0) { // to achieve idempotent INIT_JOIN message handling
            clients[senderName] = addrStr;

            /*
             * let join time be (3 cycles, including the span to the subsequent cycle) later
             * from the join message sending time
             */
            simtime_t current = readClock();
            double timeToCycleStart = sendTime.dbl() - cycleStart.dbl();
            // join time is calculated from the sending time
            double joinTime =
                    timeToCycleStart >= 0 ?
                            (sendTime.dbl() + roundCycle.dbl()
                                    - fmod(timeToCycleStart, roundCycle.dbl())
                                    + 2 * roundCycle.dbl()) :
                            (cycleStart.dbl() + 3 * roundCycle.dbl());
            double recvTime = joinTime + roundCycle.dbl();
            int joinRound = static_cast<int>(ceil(
                    (recvTime - cycleStart.dbl()) / roundCycle.dbl()));
            //    NSenders[senderId] = seq_s + 1 - joinRound;
            senders[senderId] = seq_s + 1 - joinRound;
            // time event receiving start
            Nc[senderId] = joinRound;
            JL[senderId] = seq_s;
            // add update dissemination recipient
            //    recipients[senderId] = 0 - joinRound;
            Nr[senderId] = joinRound;
            map<string, string> addresses;
            addresses[clients[senderName]] = senderName;
            //    tuple_rg rg(addresses, -1);
            Rg[senderId] = addresses;

            cout << "[" << simTime() << "s] " << "[debug] cycle start: "
                    << cycleStart << endl;
            cout << "[" << simTime() << "s] " << "[debug] message timestamp: "
                    << sendTime << endl;
            cout << "[" << simTime() << "s] " << "[debug] current time: "
                    << current << endl;
            cout << "[" << simTime() << "s] "
                    << "[debug] join sequence (seq_s + 1): " << seq_s + 1
                    << endl;
            cout << "[" << simTime() << "s] " << "[debug] join time (t_s): "
                    << joinTime << endl;
            cout << "[" << simTime() << "s] "
                    << "[debug] receive time (t_recv): " << recvTime << endl;
            cout << "[" << simTime() << "s] " << "[debug] join round: "
                    << joinRound << endl;

            Join* join = new Join(msg::JOIN);
            join->setSenderName(fullName);
            join->setCycleLength(roundCycle);
            join->setJoinTime(joinTime);
            join->setLCName(initJoin->getLCName());
            //    join->setJoinTime(cycleStart - roundCycle);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            join->setControlInfo(udpControlInfo);
            sendIdeal(join);

            app->initApp((unsigned short) clientId);
        }
    }
    delete initJoin;
}

void NodeCtrl::handleHandShake(HandShake* hs) {

//    dout << "handleHandShake" << endl;

    UDPControlInfo* hsCtrl = check_and_cast<UDPControlInfo *>(
            hs->getControlInfo());
    IPvXAddress srcAddr = hsCtrl->getSrcAddr();
    string addrStr = srcAddr.get4().str();
    if (cycleStart < 0 || LCName.empty()) {
        HandShakeReject* reject = new HandShakeReject(msg::HANDSHAKE_REJECT);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        reject->setControlInfo(udpControlInfo);
        reject->setLCName(hs->getLCName());
        sendFixedDelay(reject);

//        dout << "received HandShakeTimeout, server not initialized" << endl;

    } else {
        int seq_s = hs->getSeq();
        unsigned long senderId = hs->getSenderId();
        string senderName = hs->getSourceName();
        simtime_t sendTime = hs->getJoinTime();
        if (clients.count(senderName) == 0) { // to achieve idempotent INIT_JOIN message handling
            clients[senderName] = addrStr;

            /*
             * let join time be (3 cycles, including the span to the subsequent cycle) later
             * from the join message sending time
             */
            simtime_t current = readClock();
            double timeToCycleStart = sendTime.dbl() - cycleStart.dbl();
            // join time is calculated from the sending time
            double joinTime =
                    timeToCycleStart >= 0 ?
                            (sendTime.dbl() + roundCycle.dbl()
                                    - fmod(timeToCycleStart, roundCycle.dbl())
                                    + 2 * roundCycle.dbl()) :
                            (cycleStart.dbl() + 3 * roundCycle.dbl());
            double recvTime = joinTime + roundCycle.dbl();
            int joinRound = static_cast<int>(ceil(
                    (recvTime - cycleStart.dbl()) / roundCycle.dbl()));
            //    NSenders[senderId] = seq_s + 1 - joinRound;
            senders[senderId] = seq_s + 1 - joinRound;
            // time event receiving start
            Nc[senderId] = joinRound;
            JL[senderId] = seq_s;
            // add update dissemination recipient
            //    recipients[senderId] = 0 - joinRound;
            Nr[senderId] = joinRound;
            map<string, string> addresses;
            addresses[clients[senderName]] = senderName;
            //    tuple_rg rg(addresses, -1);
            Rg[senderId] = addresses;

//            cout << "[" << simTime() << "s] " << "[debug] cycle start: "
//                    << cycleStart << endl;
//            cout << "[" << simTime() << "s] " << "[debug] message timestamp: "
//                    << sendTime << endl;
//            cout << "[" << simTime() << "s] " << "[debug] current time: "
//                    << current << endl;
//            cout << "[" << simTime() << "s] "
//                    << "[debug] join sequence (seq_s + 1): " << seq_s + 1
//                    << endl;
//            cout << "[" << simTime() << "s] " << "[debug] join time (t_s): "
//                    << joinTime << endl;
//            cout << "[" << simTime() << "s] "
//                    << "[debug] receive time (t_recv): " << recvTime << endl;
//            cout << "[" << simTime() << "s] " << "[debug] join round: "
//                    << joinRound << endl;

            Join* join = new Join(msg::JOIN);
            join->setSenderName(fullName);
            join->setCycleLength(roundCycle);
            join->setJoinTime(joinTime);
            join->setLCName(hs->getLCName());
            //    join->setJoinTime(cycleStart - roundCycle);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            join->setControlInfo(udpControlInfo);
            sendIdeal(join);
        }

//        dout << "add new neighbor and reply JOIN" << endl;
    }

    delete hs;
}

void NodeCtrl::receiveEvent(Event* event) {
    unsigned long senderId = event->getSenderId();
    int seq = event->getSeq();
    string content = event->getContent();
    tuple_qr t(senderId, seq);
//    int MinSeq = JL[senderId];
    Qr[t] = content;

    delete event;
}

void NodeCtrl::collectRoundEvents() {
    if (groupId >= 0) {
        round++;
        EV << fullName << " round: " << round << endl;

        for (map<unsigned long, int>::iterator it = senders.begin();
                it != senders.end(); it++) {
            unsigned long senderId = it->first;

            // if the start round (i.e., the first round) of the sender hasn't been reached, skip the sender
            if (Nc[senderId] > round) {
                continue;
            }

            // determine the sequence of round: seq_s + (r - r_s) where r = round
            int sor = it->second + round;

//            cout << "[" << simTime() << "s] " << fullName << " round: " << round << " sor: " << sor << endl;

            // check whether the event of the round has been received
            tuple_qr qrKey(senderId, sor);
            tuple_qp qpKey(round, senderId, sor);
            if (Qr.count(qrKey) > 0) {
                Qp[qpKey] = Qr[qrKey];
                // remove events from Qr
                Qr.erase(qrKey);

            } else {
                Qp[qpKey] = event::MISS_EVENT;
            }
        }

        // collect events of previous rounds, until the timed round (which needs more observation)
        for (map<unsigned long, int>::iterator it = senders.begin();
                it != senders.end(); it++) {
            unsigned long senderId = it->first;

            /*
             * JL = (seq_s - r_s) + r_JL
             * => r_JL = JL - (seq_s - r_s)
             * => r_JL = JL - it->second
             * r_JL + 1: the round number corresponding to the given JL
             */
            for (int i = (JL[senderId] - it->second) + 1; i < round; i++) {
                // if the start round (i.e., the first round) of the sender hasn't been reached, skip the sender
                if (Nc[senderId] > i) {
                    continue;
                }

                // determine the sequence of round: seq_s + (r - r_s) where round cycle r = i and seq_s - r_s = it->second
                int sor = it->second + i;

                tuple_qr qrKey(senderId, sor);
                tuple_qp qpKey(i, senderId, sor);
                if (Qr.count(qrKey) > 0) {
                    Qp[qpKey] = Qr[qrKey];
                    // remove events from Qr
                    Qr.erase(qrKey);
                }
                /*
                 * marking previous missing events may not needed, since a needed event not in Qp will be marked by
                 * MISS_EVENT in event delivery
                 */
//                else{
//                    Qp[qpKey] = event::MISS_EVENT;
//                }
            }
        }

        // check and handle client interruption
        if (senders.count(clientId) > 0 && Nc[clientId] <= round) {
            int sor = senders[clientId] + round;
            tuple_qp qpKey(round, clientId, sor);
            string clientEvent = Qp[qpKey];
            if (clientEvent.compare(event::MISS_EVENT) == 0) {

//                dout << fullName << " failed to receive a client event" << endl;

                if (!suspect_client) {

//                    dout << "fullName" << " start to suspect the client"
//                            << endl;

                    suspect_client = true;
                    suspectStart = simTime();
                } else {
                    simtime_t suspectDuration = simTime() - suspectStart;

//                    dout << "fullName" << " suspectDuration: "
//                            << suspectDuration << endl;
//                    dout << "fullName" << " client_interruption: "
//                            << client_interruption << endl;

                    if (suspectDuration > client_interruption) {
                        // start to terminate
                        receiveClient = false;
                        is_ccto = true;
                        app->onExit();
                    }
                }
            } else {
                suspect_client = false;
            }
        }

        // prepare for the next cycle
        next_round_time += roundCycle;
        scheduleAt(next_round_time, ECTimeout);

        // handle event
        deliverCycleEvent();
    }
}

void NodeCtrl::deliverCycleEvent() {

    while (round > deliveryRound && mode == EH) {
        deliveryRound++;

        // check whether round r is ready, to reduce the number of messages sent to leader
        if (proposed.count(deliveryRound) > 0) {
            deliveryRound--;
            break;
        }

        // tuple (round#, sender ID, sequence number, event content)
        typedef boost::tuple<int, unsigned long, int> tuple_d;
        map<tuple_d, string> Delivery;

        if (Qc.count(deliveryRound) > 0) {
            pair<multimap<int, tuple_qc>::iterator,
                    std::multimap<int, tuple_qc>::iterator> ret =
                    Qc.equal_range(deliveryRound);
            for (multimap<int, tuple_qc>::iterator it = ret.first;
                    it != ret.second; ++it) {
                int key = it->first;
                tuple_qc tuple = it->second;
                tuple_d dKey(key, get<0>(tuple), get<1>(tuple));
                Delivery[dKey] = get<2>(tuple);
            }
        } else {
            // tuple (round#, sender ID, sequence number, event content)
            typedef boost::tuple<int, unsigned long, int> tuple_e;
            map<tuple_e, string> Events;

            for (map<unsigned long, int>::iterator it = senders.begin();
                    it != senders.end(); it++) {
                unsigned long senderId = it->first;
                if (Nc[senderId] > deliveryRound) {
                    continue;
                }
                // calculate the lowest sequence number of the sender
                int lowestDeliverableSeq = JL[senderId] + 1;
                // calculate the sequence number of the round
                int sor = it->second + deliveryRound;
                for (int i = lowestDeliverableSeq; i <= sor; i++) {
                    int r = deliveryRound - (sor - i);
                    tuple_e eKey(deliveryRound, senderId, i);
                    tuple_qp pKey(r, senderId, i);
                    if (Qp.count(pKey) > 0) {
                        Events[eKey] = Qp[pKey];
                    } else {
                        // in case of new replica join
                        Events[eKey] = event::MISS_EVENT;
                    }
                }
            }

            // check whether Events contains any missing event
            bool completeRound = true;
            for (map<tuple_e, string>::iterator it = Events.begin();
                    it != Events.end(); it++) {
                string event = it->second;
                if (event.compare(event::MISS_EVENT) == 0) {
                    completeRound = false;
                    break;
                }
            }
            if (!completeRound) {
                round_query++;
                query_round.insert(deliveryRound);

                // calculate the range of sequence number for each sender for query
                stringstream ss;
                for (map<unsigned long, int>::iterator it = senders.begin();
                        it != senders.end(); it++) {
                    unsigned long senderId = it->first;
                    if (Nc[senderId] > deliveryRound) {
                        continue;
                    }
                    int lowestDeliverableSeq = JL[senderId] + 1;
                    int sor = it->second + deliveryRound;
                    ss << senderId << "," << lowestDeliverableSeq << "," << sor
                            << ";";
                }
                string seqRanges = ss.str();
                // If contain any missing event, send QUERY to coordinator to trigger a consensus instance
                RoundQuery* query = new RoundQuery(msg::ROUND_QUERY);
                query->setSenderName(fullName);
                query->setDestName(coord.c_str());
                query->setRound(deliveryRound);
                query->setSeqRanges(seqRanges.c_str());
                // message size: size of seqRange and 1 byte for round number
                query->setByteLength(seqRanges.size() + 1);
                UDPControlInfo* udpControlInfo = new UDPControlInfo();
                IPvXAddress coordAddr = getIPAddress(coord);
                udpControlInfo->setDestAddr(coordAddr);
                udpControlInfo->setSrcAddr(ipAddress);
                query->setControlInfo(udpControlInfo);
                r_transmit(query, coordAddr);
                // mark the round undeliverable
                deliveryRound--;
                break;
            } else {
                Delivery.insert(Events.begin(), Events.end());
            }
        }
        if (!Delivery.empty()) {
            // deliver events to Qd
            for (map<tuple_d, string>::iterator it = Delivery.begin();
                    it != Delivery.end(); it++) {
                tuple_d dKey = it->first;
                int r = get<0>(dKey);
                unsigned long senderId = get<1>(dKey);
                int seq = get<2>(dKey);
                string event = it->second;
                tuple_qd key(r, senderId, seq);
                Qd[key] = event;
                // update JL
                if (event.compare(event::EMPTY_EVENT) != 0
                        && (JL.count(senderId) == 0 || JL[senderId] < seq)) {
                    JL[senderId] = seq;
                }
            }

            // whether the following codes are correct needs more observation
            // garbage collection (GC) on Qc, and Qp
            set<int> QcKeys;
            for (multimap<int, tuple_qc>::iterator it = Qc.begin();
                    it != Qc.end(); it++) {
                int QcKey = it->first;
                if (QcKey <= deliveryRound) {
                    QcKeys.insert(QcKey);
                }
            }
            for (set<int>::iterator it = QcKeys.begin(); it != QcKeys.end();
                    it++) {
                int QcKey = *it;
                Qc.erase(QcKey);
            }

            set<tuple_qp> QpKeys;
            /*
             * garbage collection on Qp should refers to JL instead of deliveryRound,
             * since the missing cycle events still need to be queried in consensus
             */
            for (auto elem : JL) {
                unsigned long senderId = elem.first;
                int minSeq = elem.second;
                for (auto elem_qp : Qp) {
                    unsigned long senderId_QpKey = get<1>(elem_qp.first);
                    int seq_QpKey = get<2>(elem_qp.first);
                    if (senderId_QpKey == senderId && seq_QpKey <= minSeq) {
                        QpKeys.insert(elem_qp.first);
                    }
                }
            }
            for (set<tuple_qp>::iterator it = QpKeys.begin();
                    it != QpKeys.end(); it++) {
                tuple_qp QpKey = *it;
                Qp.erase(QpKey);
            }
            /*
             * To avoid a plethora of upper-stream event application call
             */
//            applyEvent();
        }

    }
    /*
     * for immediate event apply after new replica join, move the following function out from the while loop
     */
    if (mode == EH) {
        applyEvent();
    }
}

void NodeCtrl::handleEvent(CycleEvent* ce) {
    // TODO
}

void NodeCtrl::handleRoundQuery(RoundQuery* query) {
    UDPControlInfo *queryCtrl = check_and_cast<UDPControlInfo *>(
            query->getControlInfo());
    IPvXAddress& srcAddr = queryCtrl->getSrcAddr();
    int round = query->getRound();
    string sender = query->getSenderName();
    RoundQueryReply* rqr = NULL;
    string senderSeqRanges = query->getSeqRanges();
    // tuple(sender ID, lowerest sequence number, sequene of the round)
    typedef boost::tuple<int, int> tuple_seqRange;
    map<unsigned long, tuple_seqRange> senderInfos;
    util::unpackSenderSeqRanges(senderSeqRanges, senderInfos);

    // tuple (round#, sender ID, sequence number, event content)
    typedef boost::tuple<int, unsigned long, int> tuple_r;
    map<tuple_r, string> Responses;
    for (map<unsigned long, tuple_seqRange>::iterator it = senderInfos.begin();
            it != senderInfos.end(); it++) {
        // retrieve the range of sequence number
        unsigned long senderId = it->first;
        tuple_seqRange seqs = it->second;
        int lowestDeliverableSeq = get<0>(seqs);
        int sor = get<1>(seqs);
        for (int i = lowestDeliverableSeq; i <= sor; i++) {
            int r = round - (sor - i);
            tuple_r eKey(round, senderId, i);
            tuple_qp pKey(r, senderId, i);
            tuple_qd key(round, senderId, i);
            if (Qd.count(key) > 0) {
                Responses[eKey] = Qd[key];
            } else if (Qp.count(pKey) > 0) {
                Responses[eKey] = Qp[pKey];
            } else {
                // round event collection not yet reached
                delete query;
                return;
            }
        }
    }

    // check whether Events contains any missing event
    bool completeRound = true;
    for (map<tuple_r, string>::iterator it = Responses.begin();
            it != Responses.end(); it++) {
        string event = it->second;
        if (event.compare(event::MISS_EVENT) == 0) {
            completeRound = false;
            break;
        }
    }
    if (!completeRound) {
        // Trigger a consensus instance
        if (toPropose.count(round) == 0) {
            toPropose[round] = senderSeqRanges;
            initConsensus();
        }
    } else {
        rqr = new RoundQueryReply(msg::ROUND_QUERY_REPLY);
        string events = util::convertMapToStr(Responses);
        rqr->setEvents(events.c_str());
        rqr->setSenderName(fullName);
        rqr->setDestName(sender.c_str());
        rqr->setRound(round);
        // message size: events size + 1 byte for round
        rqr->setByteLength(events.size() + 1);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        rqr->setControlInfo(udpControlInfo);
        r_transmit(rqr, srcAddr);
    }

    delete query;
}

void NodeCtrl::handleRoundQueryReply(RoundQueryReply* rqr) {
    int round = rqr->getRound();
    string eventStr = rqr->getEvents();
    typedef boost::tuple<int, unsigned long, int> tuple_e;
    map<tuple_e, string> events;
    util::convertStrToMap(eventStr, events);
    for (map<tuple_e, string>::iterator it = events.begin(); it != events.end();
            it++) {
        tuple_e eKey = it->first;
        string event = it->second;
        tuple_qc content(get<1>(eKey), get<2>(eKey), event);
        Qc.insert(pair<int, tuple_qc>(round, content));
    }

    deliverCycleEvent();

    delete rqr;
}

void NodeCtrl::initConsensus() {
    if (mode == EH && coord.compare(ipAddress.get4().str()) == 0) {
        for (map<int, string>::iterator it = toPropose.begin();
                it != toPropose.end(); it++) {
            int r = it->first;
            if (proposed.count(r) == 0) {
                proposed.insert(r);
                ConsensusQuery* cq = new ConsensusQuery(msg::CONSENSUS_QUERY);
                cq->setSenderName(fullName);
                cq->setEpoch(epoch);
                cq->setGroupId(groupId);
                cq->setRound(r);
                string senderSeqRanges = it->second;
                cq->setSeqRanges(senderSeqRanges.c_str());
                // message size: events size + 1 byte for round + 1 byte for epoch + 1 byte for group ID
                cq->setByteLength(senderSeqRanges.size() + 3);
                r_multicast(cq, survivedMembers());

                GlobalStatisticsAccess().get()->increaseConsensusStart();
            }
        }
    }
}

void NodeCtrl::startPropose(ConsensusQuery * cq) {
    UDPControlInfo *cqCtrl = check_and_cast<UDPControlInfo *>(
            cq->getControlInfo());
    IPvXAddress& srcAddr = cqCtrl->getSrcAddr();
    int r = cq->getRound();
    string senderSeqRanges = cq->getSeqRanges();
    int e = cq->getEpoch();
    int gid = cq->getGroupId();
    // since this query changes "proposed", the checks on epoch and configuration ID is needed
    if (e == epoch && gid == groupId && mode == EH) {
        string received = roundEToString(r, senderSeqRanges);

        if (hasGUI()) {
//            cout << "[" << simTime() << "][" << fullName
//                    << "] Create the proposal: " << received << endl;
        }

        if (!received.empty()) {
            proposed.insert(r);

            Proposal* proposal = new Proposal(msg::PROPOSAL);
            proposal->setSenderName(fullName);
            proposal->setEpoch(epoch);
            proposal->setGroupId(groupId);
            proposal->setRound(r);
            proposal->setReceived(received.c_str());
            // message size: received event size + 1 byte for round + 1 byte for epoch + 1 byte for group ID
            proposal->setByteLength(received.size() + 3);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            proposal->setControlInfo(udpControlInfo);
            r_transmit(proposal, srcAddr);
        } else {
            // in case the events of the given cycle haven't been received
            ConsensusNAck* nack = new ConsensusNAck(msg::CONSENSUS_NACK);
            nack->setSenderName(fullName);
            nack->setEpoch(epoch);
            nack->setGroupId(groupId);
            nack->setRound(r);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            nack->setControlInfo(udpControlInfo);
            r_transmit(nack, srcAddr);
        }
    } else {
        // for debug purpose
        EV << "[" << simTime() << "s] " << fullName
                  << " cannot start a new consensus instance" << endl;
        if (mode != EH) {
            EV << "[" << simTime() << "s] " << fullName
                      << " is not in the event handling mode" << endl;
        } else {
            EV << "[" << simTime() << "s] " << fullName
                      << " has not finished the last consensus instance"
                      << endl;
        }

        ConsensusNAck* nack = new ConsensusNAck(msg::CONSENSUS_NACK);
        nack->setSenderName(fullName);
        nack->setEpoch(epoch);
        nack->setGroupId(groupId);
        nack->setRound(r);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        nack->setControlInfo(udpControlInfo);
        r_transmit(nack, srcAddr);
    }

    delete cq;
}

string NodeCtrl::roundEToString(int queryRound, string senderSeqRanges) {
    string roundEvents;

    // tuple(sender ID, lowerest sequence number, sequene of the round)
    typedef boost::tuple<int, int> tuple_seqRange;
    map<unsigned long, tuple_seqRange> senderInfos;
    util::unpackSenderSeqRanges(senderSeqRanges, senderInfos);

    // tuple (round#, sender ID, sequence number, event content)
    typedef boost::tuple<int, unsigned long, int> tuple_r;
    map<tuple_r, string> Responses;
    for (map<unsigned long, tuple_seqRange>::iterator it = senderInfos.begin();
            it != senderInfos.end(); it++) {
        // retrieve the range of sequence number
        unsigned long senderId = it->first;
        tuple_seqRange seqs = it->second;
        int lowestDeliverableSeq = get<0>(seqs);
        int sor = get<1>(seqs);
        for (int i = lowestDeliverableSeq; i <= sor; i++) {
            int r = queryRound - (sor - i);
            tuple_r eKey(queryRound, senderId, i);
            tuple_qp pKey(r, senderId, i);
            tuple_qd key(queryRound, senderId, i);
            if (Qd.count(key) > 0) {
                Responses[eKey] = Qd[key];
            } else if (Qp.count(pKey) > 0) {
                Responses[eKey] = Qp[pKey];
            } else if (round >= queryRound) {
                // to avoid live lock, in case of a new replica, it may have a fresh Qp
                Responses[eKey] = event::MISS_EVENT;
            } else {
                // this should not occur unless clock drift
                Responses.clear();
                goto endloop;
            }
        }
    }

    endloop: // end of the entire loop
    if (!Responses.empty()) {
        roundEvents = util::convertMapToStr(Responses);
    }

    return roundEvents;
}

void NodeCtrl::collectProposal(Proposal* proposal) {
    UDPControlInfo *proposalCtrl = check_and_cast<UDPControlInfo *>(
            proposal->getControlInfo());
    IPvXAddress& srcAddr = proposalCtrl->getSrcAddr();
    string senderAddr = srcAddr.get4().str();
    string senderName = proposal->getSenderName();

    EV << "[" << simTime() << "s] " << fullName << " received PROPOSAL from "
              << senderName << " [" << senderAddr << "]" << endl;

    int e = proposal->getEpoch();
    int gid = proposal->getGroupId();
    string received = proposal->getReceived();
    int r = proposal->getRound();

    if (e == epoch && gid == groupId && mode == EH) {
        typedef boost::tuple<int, unsigned long, int> tuple_e;
        map<tuple_e, string> events;
        if (!received.empty()) {
            util::convertStrToMap(received, events);
        }
        map<tuple_ppKey, string> eTuples;
        for (map<tuple_e, string>::iterator it = events.begin();
                it != events.end(); it++) {
            tuple_e eKey = it->first;
            string event = it->second;
            tuple_ppKey ppKey(get<1>(eKey), get<2>(eKey));
            eTuples[ppKey] = event;
        }
        tuple_pKey pKey(r, senderAddr);
        proposals[pKey] = eTuples;
    }

    checkProposals(r);

    delete proposal;
}

void NodeCtrl::handleConsensusNAck(ConsensusNAck* nack) {
    /*
     * reset proposed and proposals, but cannot reset toPropose,
     * since the event for consensus has already in the "proposed" list of other non-leader replicas,
     * which will prevent them from further event delivery
     */
    nack->getSenderName();
    int cycle = nack->getRound();
    proposed.erase(cycle);
    set<tuple_pKey> keys;
    for (auto elem : proposals) {
        tuple_pKey key = elem.first;
        int proposedCycle = get<0>(key);
        if (proposedCycle == cycle) {
            keys.insert(key);
        }
    }
    for (auto elem : keys) {
        proposals.erase(elem);
    }

    delete nack;
}

void NodeCtrl::checkProposals(int r) {
    if (mode == EH) {
        bool allReceived = true;
        set<string> memberList = survivedMembers();
        // check whether all replies from  hosts on the Survivals list have been received
        for (set<string>::iterator it = memberList.begin();
                it != memberList.end(); it++) {
            string host = *it;
            tuple_pKey pKey(r, host);
            if (proposals.count(pKey) == 0) {
                allReceived = false;
                break;
            }
        }

        if (allReceived) {
            EV << "[" << simTime() << "s] " << fullName
                      << " start processing proposals" << endl;

            map<boost::tuple<int, unsigned long, int>, string> dec = decide(r,
                    memberList);

            // create DECISION message
            Decision* decisionMsg = new Decision(msg::DECISION);
            decisionMsg->setSenderName(fullName);
            decisionMsg->setEpoch(epoch);
            decisionMsg->setGroupId(groupId);
            decisionMsg->setRound(r);

            string decision = "";
            if (!dec.empty()) {
                decision = util::convertMapToStr(dec);
            }
            decisionMsg->setDecision(decision.c_str());
            // message size: decided event size + 1 byte for round + 1 byte for epoch + 1 byte for group ID
            decisionMsg->setByteLength(decision.size() + 3);

            // broadcast DECISION message
            r_multicast(decisionMsg, memberList);

            // empty proposals
            for (set<string>::iterator it = memberList.begin();
                    it != memberList.end(); it++) {
                string host = *it;
                tuple_pKey pKey(r, host);
                proposals.erase(pKey);
            }
        }
    }
}

map<boost::tuple<int, unsigned long, int>, string> NodeCtrl::decide(int round,
        set<string> memberList) {
    // tuple(sender ID, lowerest sequence number, sequene of the round)
    typedef boost::tuple<int, int> tuple_seqRange;
    map<unsigned long, tuple_seqRange> senderInfos;
    util::unpackSenderSeqRanges(toPropose[round], senderInfos);

    // tuple (round#, sender ID, sequence number, event content)
    map<boost::tuple<int, unsigned long, int>, string> Decisions;
    for (map<unsigned long, tuple_seqRange>::iterator it = senderInfos.begin();
            it != senderInfos.end(); it++) {
        // retrieve the range of sequence number
        unsigned long senderId = it->first;
        tuple_seqRange seqs = it->second;
        int lowestDeliverableSeq = get<0>(seqs);
        int sor = get<1>(seqs);
        for (int j = lowestDeliverableSeq; j <= sor; j++) {
            for (set<string>::iterator it = memberList.begin();
                    it != memberList.end(); it++) {
                string host = *it;
                tuple_pKey pKey(round, host);
                map<tuple_ppKey, string> events = proposals[pKey];
                tuple_ppKey ppKey(senderId, j);
                string event = events[ppKey];
                boost::tuple<int, unsigned long, int> dKey(round, senderId, j);

//                EV << "[Debug] host = " << hostName << " senderId = "
//                        << senderId << " seq = " << j << endl;

                if (Decisions.count(dKey) == 0
                        || Decisions[dKey].compare(event::EMPTY_EVENT) == 0) {
                    if (event.compare(event::MISS_EVENT) != 0) {
                        Decisions[dKey] = event;

//                        EV << "[Debug] non-missing event = " << event << endl;

                    } else {
                        Decisions[dKey] = event::EMPTY_EVENT;
//                        EV << "[Debug] missing event = " << event << endl;
                    }
                } else {
//                    EV << "[Debug] event already decided" << endl;
                }
            }

        }
    }

    return Decisions;
}

void NodeCtrl::handleDecision(Decision* decision) {
    // validate message
    string sender = decision->getSenderName();
    int r = decision->getRound();
    EV << "[" << simTime() << "s] " << fullName << " received decision from "
              << sender << endl;

    int e = decision->getEpoch();
    int gid = decision->getGroupId();
    string dec = decision->getDecision();
    if (e == epoch && gid == groupId && mode == EH) {

        toPropose.erase(r);
        proposed.erase(r);

        if (!dec.empty()) {
            // tuple (round#, sender Id, sequence#)
            typedef boost::tuple<int, unsigned long, int> tuple_e;
            map<tuple_e, string> roundEvents;
            util::convertStrToMap(dec, roundEvents);
            for (map<tuple_e, string>::iterator it = roundEvents.begin();
                    it != roundEvents.end(); it++) {
                tuple_e eKey = it->first;
                string event = it->second;
                tuple_qc content(get<1>(eKey), get<2>(eKey), event);
                Qc.insert(pair<int, tuple_qc>(get<0>(eKey), content));
            }

            deliverCycleEvent();
        }

        // for statistics on the total number of consensus in a simulation run
        if (coord.compare(ipAddress.get4().str()) == 0) {
            GlobalStatisticsAccess().get()->increaseConsensusEnd();
        }
    }

    delete decision;
}

void NodeCtrl::applyEvent() {
    // serialize events in Qd, deliver non-empty events, and remove empty events
    map<tuple_qd, string>::iterator it = Qd.begin();
    if (get<0>(lastDelivered) != -1) {
        tuple_qd key(get<0>(lastDelivered), get<1>(lastDelivered),
                get<2>(lastDelivered));
        if (Qd.find(key) != Qd.end()) {
            it = Qd.find(key);
            it++;
        }
    }
    for (; it != Qd.end(); it++) {
        tuple_qd key = it->first;
        string event = it->second;
        // exclude missing/empty events
        if (event.find(event::TO_EXIT) < event.length()) {
            vector<string> container;
            util::splitString(event, "|", container);
            event = container[0];
            receiveClient = false;
        }
        if (event.compare(event::EMPTY_EVENT) != 0
                && event.compare(event::NO_INPUT) != 0) {
            EventApply* apply = new EventApply(msg::EVENT_APPLY);
            apply->setEvent(event.c_str());
            apply->setRound(get<0>(key));
            apply->setClientId(get<1>(key));
            apply->setSequence(get<2>(key));
            send(apply, this->gateHalf("uplink", cGate::OUTPUT));
        }
        if (event.find(event::TO_REMOVE) == 0) {
            vector<string> container;
            util::splitString(event, "+", container);
            string senderName = container[2];
            unsigned long senderId = util::strToLong(container[3]);
            senders.erase(senderId);
            JL.erase(senderId);
            Nc.erase(senderId);
            clients.erase(senderName);
            Nr.erase(senderId);
            Rg.erase(senderId);
        }
        lastDelivered = boost::tuple<int, unsigned long, int>(get<0>(key),
                get<1>(key), get<2>(key));
    }
}

void NodeCtrl::applyUpdate(UpdateApply* updateApply) {
    string event = updateApply->getEvent();
    unsigned long clientId = updateApply->getClientId();
    tuple_qd key(updateApply->getRound(), clientId, updateApply->getSequence());
    state.first = key;
    state.second = updateApply->getState();

    vector<string> container;

//    cout << "event to reply: " << event << endl;

    util::splitString(event, "+", container);

    // broadcast handled events to recipients
    for (map<unsigned long, int>::iterator it = Nr.begin(); it != Nr.end();
            it++) {
        unsigned long recipientId = it->first;
        if (it->second > round) {
            continue;
        }

        // destination specified events
        if (event.find(event::GET_BLOCK) < event.length()) {

//            cout << "GET_BLOCK event at applyUpdate" << endl;

            unsigned long dest = util::strToLong(container[4]);
            // Only the responsible Mesh replies.

            if (dest != recipientId) {

//                cout << LCName << " Reply GET_BLOCK to the caller" << endl;
                continue;
            }
        } else if (event.find(event::TO_REMOVE) < event.length()) {

//            dout << LCName << " TO_REMOVE event at applyUpdate" << endl;

            if (this->clientId != recipientId) {
                continue;
            }
        } else if (event.find(event::TO_JOIN) < event.length()) {
            // actually, TO_JOIN is the event of TOCLIENT_PLAYERPOS

//            cout << "update position event at applyUpdate" << endl;

            if (this->clientId == recipientId
                    && event.find("NPC") >= event.length()) {
                continue;
            }
        } else if (event.find(event::TO_INIT) < event.length()) {

//            cout << "Init client state at applyUpdate" << endl;
//            dout << "Init client state at applyUpdate" << endl;

            if (this->clientId != recipientId) {
                continue;
            }
        }

//        cout << LCName << " send event " << event << " to " << recipientId
//                << endl;

        Update* update = new Update(msg::UPDATE);
        update->setContent(event.c_str());
        update->setLCName(LCName.c_str());
        update->setSenderId(clientId);
        update->setSequence(updateApply->getSequence());
        update->setSourceName(fullName);
        // message size: content size + 1 byte for sender ID + byte for sequence number + 1 byte for logical computer identity
        update->setByteLength(event.size() + 1 + 1 + 1);
        map<string, string> addresses = Rg[recipientId];
        for (map<string, string>::iterator it3 = addresses.begin();
                it3 != addresses.end(); it3++) {
            Update* updateDup = update->dup();
            IPvXAddress dest(it3->first.c_str());
            string hostName = it3->second;
            updateDup->setDestName(hostName.c_str());
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(dest);
            udpControlInfo->setSrcAddr(ipAddress);
            updateDup->setControlInfo(udpControlInfo);
            sendReliable(updateDup);
        }

        delete update;
    }

    delete updateApply;
}

void NodeCtrl::disseminateEvent() {
    // periodically disseminate updates to interested secondary copies

    // prepare for the next cycle
//    scheduleAt(readClock() + roundCycle, updateTimeout);
}

void NodeCtrl::gossip() {
    Gossip* gossip = new Gossip(msg::GOSSIP_GC);
    gossip->setSourceName(fullName);
    gossip->setSendTime(readClock());
    gossip->setLastRound(deliveryRound);
    // broadcast DECISION message
    r_multicast(gossip, survivedMembers());

    // reset gossip timer
    scheduleAt(readClock() + gcCycle, GCTimeout);

    emit(QdLength, (double) Qd.size());
}

void NodeCtrl::handleGossip(Gossip* gossip) {
    if (groupId >= 0) {
        UDPControlInfo *gossipCtrl = check_and_cast<UDPControlInfo *>(
                gossip->getControlInfo());
        IPvXAddress& srcAddr = gossipCtrl->getSrcAddr();
        string sender = srcAddr.get4().str();
        simtime_t ts = gossip->getSendTime();
        int lastRound = gossip->getLastRound();
        if (histories.count(sender) == 0 || ts > get<1>(histories[sender])) {
            histories[sender] = tuple_h(lastRound, ts);
        }

        set<string> survived_members = survivedMembers();

        // remove failed replicas in the histories buffer
        set<string> obsolete;
        for (auto elem : histories) {
            if (survived_members.count(elem.first) == 0) {
                obsolete.insert(elem.first);
            }
        }
        for (auto elem : obsolete) {
            histories.erase(elem);
        }

        // If histories contains the messages from all the survivedMembers, then garbage collection can proceed.
        if (includes(histories | map_keys, survived_members)) {
            vector<int> lastRounds;
            for (map<string, tuple_h>::iterator it = histories.begin();
                    it != histories.end(); it++) {
                lastRounds.push_back(get<0>(it->second));
            }
            int lastCommRound = *min_element(lastRounds.begin(),
                    lastRounds.end());
            vector<tuple_qd> purge_qd;
            int lastDeliveredRound = get<0>(lastDelivered);
            int lastRemove =
                    lastCommRound <= lastDeliveredRound ?
                            lastCommRound : lastDeliveredRound;
            for (map<tuple_qd, string>::iterator it = Qd.begin();
                    it != Qd.end(); it++) {
                tuple_qd key = it->first;
                int r = get<0>(key);
                if (r <= lastRemove) {
                    purge_qd.push_back(key);
                }
            }
            for (size_t i = 0; i < purge_qd.size(); i++) {
                Qd.erase(purge_qd[i]);
            }
        }
    }

    delete gossip;
}

void NodeCtrl::stateTransfer() {
    if (groupId >= 0) {
        // check whether coordinator change needs to be triggered
        bool triggerCood = checkCoord();
        string addrStr = ipAddress.get4().str();
        if (!triggerCood) {
            if (mode == CC) {
                if (coordCand.compare(addrStr) == 0) {
                    checkCCSnapshos();
                    checkCCAcks();
                }
            } else {
                // check whether group reconfiguration needs to be triggered
                bool triggerReconfig = checkConfig();
                if (!triggerReconfig) {
                    if (mode == GR) {
                        if (coord.compare(addrStr) == 0) {
                            checkGRSnapshots();
                        }
                    } else {
                        if (coord.compare(addrStr) == 0) {
                            /*
                             * run the following function for each round r, in case that the waiting replica becomes failed
                             */
                            initConsensus();
                            for (set<int>::iterator it = proposed.begin();
                                    it != proposed.end(); it++) {
                                checkProposals(*it);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool NodeCtrl::checkCoord() {
    string addrStr = ipAddress.get4().str();
    /*
     * check whether coordinator is survived:
     * 1) not a new node;
     * 2) not a coordinator or coordinator candidate;
     * 3) the current coordinator does not survive.
     */
    if (seniorities[addrStr] > 0 && coordCand.compare(addrStr) != 0
            && ((coord.compare(addrStr) != 0)
                    && survivals.find(coord) == survivals.end())) {
        EV << "[" << simTime() << "s] " << fullName << " found coordinator "
                  << coord << " has failed" << endl;

        // if not, go to coordinator change mode
        mode = CC;
        coord.clear();

        // compute the coordinator candidate
        string candidate = coordinator();
        string addrStr = ipAddress.get4().str();
        // if the node is a coordinator candidate
        if (candidate.compare(addrStr) == 0) {
            EV << "[" << simTime() << "s] " << fullName
                      << " requested to become the new coordinator" << endl;

            // if the node itself is the coordinator candidate
            coordCand = addrStr;
            CCSnapshots.clear();
            membersTemp.clear();

            // send coord_query to other members
            CCQuery* coordChange = new CCQuery(msg::COORD_CHANGE);
            coordChange->setCandidate(addrStr.c_str());
            r_multicast(coordChange, survivedMembers());

            GlobalStatisticsAccess().get()->increaseLeaderElectionStart();
        }
        return true;
    } else {
        return false;
    }
}

string NodeCtrl::coordinator() {
    string coordAddr = ipAddress.get4().str().c_str();
    unsigned long coordId = sdbm::encode(coordAddr.c_str());
    set<string> survived_members = survivedMembers();
    for (set<string>::iterator it = survived_members.begin();
            it != survived_members.end(); it++) {
        string addr = *it;
        unsigned long id = sdbm::encode(addr.c_str());
        if (seniorities[addr] > seniorities[coordAddr]
                || (seniorities[addr] == seniorities[coordAddr] && id > coordId)) {
            coordId = id;
            coordAddr = *it;
        }
    }
    return coordAddr;
}

void NodeCtrl::handleCCQuery(CCQuery* cc) {
    UDPControlInfo *ccCtrl = check_and_cast<UDPControlInfo *>(
            cc->getControlInfo());
    IPvXAddress& srcAddr = ccCtrl->getSrcAddr();
    string candidateAddr = cc->getCandidate();

    EV << "[" << simTime() << "s] " << fullName
              << " received coordinator change from " << candidateAddr << endl;

    string candidate = coordinator();
    if (survivals.count(candidateAddr) > 0
            && candidateAddr.compare(candidate) == 0) {
        mode = CC;
        coord.clear();

        CCQueryReply* ccr = new CCQueryReply(msg::COORD_CHANGE_REPLY);
        ccr->setSenderName(fullName);
        string snapshot = buildCCSnapshot();
        ccr->setState(snapshot.c_str());
        // message size: snapshot size
        ccr->setByteLength(snapshot.size());
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        ccr->setControlInfo(udpControlInfo);
        r_transmit(ccr, srcAddr);
    } else {
        CCNAck* ccr = new CCNAck(msg::CC_NACK);
        ccr->setSenderName(fullName);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        ccr->setControlInfo(udpControlInfo);
        r_transmit(ccr, srcAddr);
    }

    delete cc;
}

string NodeCtrl::buildCCSnapshot() {
    Value snapshot;
    string Qdstr = util::convertMapToStr(Qd);
    string Qcstr = util::convertMapToStr(Qc);
    snapshot["Qd"] = Qdstr;
    snapshot["E"] = Qcstr;
    snapshot["epoch"] = epoch;
    snapshot["GID"] = groupId;
    string memberlist = util::convertSetToStr(members, 0);
    snapshot["G"] = memberlist;

    Value lastState;
    lastState["key"]["round"] = (int) get<0>(state.first);
    lastState["key"]["clientId"] = (unsigned int) get<1>(state.first);
    lastState["key"]["sequence"] = (int) get<2>(state.first);
    lastState["state"] = (unsigned int) state.second;
    snapshot["last_state"] = lastState;
    string appState = app->packState();
    snapshot["app_state"] = appState;
    FastWriter fw;
    return fw.write(snapshot);
}

void NodeCtrl::handleCCNAck(CCNAck* ccnack) {
    UDPControlInfo *ccnackCtrl = check_and_cast<UDPControlInfo *>(
            ccnack->getControlInfo());
    IPvXAddress& srcAddr = ccnackCtrl->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = ccnack->getSenderName();
    // retransmit coordinate change request
    string candidate = coordinator();
    string addrStr = ipAddress.get4().str();
    if (candidate.compare(addrStr) == 0) {
        EV << "[" << simTime() << "s] " << fullName
                  << " retransmit coordinator change request to " << senderName
                  << endl;

        // if self is the coordinator candidate
        coordCand = addrStr;

        // send coord_query to the reply host, if the host is still a live member
        if (survivals.count(sender) > 0 && members.count(sender) > 0) {
            CCQuery* coordChange = new CCQuery(msg::COORD_CHANGE);
            coordChange->setCandidate(addrStr.c_str());
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            coordChange->setControlInfo(udpControlInfo);
            r_transmit(coordChange, srcAddr);
        }
    } else {
        coordCand.clear();
    }

    delete ccnack;
}

void NodeCtrl::handleCCQueryReply(CCQueryReply* ccr) {
    UDPControlInfo *ccrCtrl = check_and_cast<UDPControlInfo *>(
            ccr->getControlInfo());
    IPvXAddress& srcAddr = ccrCtrl->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = ccr->getSenderName();

    EV << "[" << simTime() << "s] " << fullName
              << " received coordinator change reply from " << senderName
              << endl;

    // only handle messages from survived members, TODO but this condition may not needed
    set<string> survived_members = survivedMembers();
    if (survived_members.count(sender) > 0) {
        CCSnapshots[sender] = ccr->getState();
        checkCCSnapshos();
    }

    delete ccr;
}

void NodeCtrl::checkCCSnapshos() {

    bool allReceived = true;

    set<string> survived_members = survivedMembers();
    // check whether all replies from  hosts on the Survivals list have been received
    for (set<string>::iterator it = survived_members.begin();
            it != survived_members.end(); ++it) {
        string hostName = *it;
        if (CCSnapshots.find(hostName) == CCSnapshots.end()) {
            allReceived = false;
            break;
        }
    }

    if (allReceived) {
        Value snapshot = computeCCSnapshot();
        CCEnd* cce = new CCEnd(msg::LOAD_COORD);
        cce->setSenderName(fullName);
        cce->setEpoch(snapshot["epoch"].asInt());
        bool reconfig = snapshot["reconfig"].asBool();
        cce->setReconfig(reconfig);
        if (reconfig) {
            cce->setGroupId(snapshot["GID"].asInt());
            cce->setGroupMembers(snapshot["memberlist"].asCString());
            cce->setQd(snapshot["Qd"].asCString());
            cce->setQc(snapshot["E"].asCString());
            FastWriter fw;
            cce->setInit(fw.write(snapshot["init"]).c_str());
        }
        // message size: 1 byte for epoch + snapshot size
        FastWriter fw;
        string s_snapshot = fw.write(snapshot);
        cce->setByteLength(1 + s_snapshot.size());
        r_multicast(cce, survived_members);
    }
}

Value NodeCtrl::computeCCSnapshot() {
    Value snapshot;

    bool reconfig = false;
    int highest_epoch = epoch;
    Reader reader;
    for (auto elem : CCSnapshots) {
        string replica_state = elem.second;
        Value jState;
        reader.parse(replica_state, jState);
        // retrieve the highest epoch
        int replica_epoch = jState["epoch"].asInt();
        if (replica_epoch > highest_epoch) {
            highest_epoch = replica_epoch;
        }
        /*
         * Check whether state sync is needed (i.e., last reconfiguration has been done or not).
         */
        int replica_gid = jState["GID"].asInt();
        if (replica_gid != groupId) {
            reconfig = true;
        }
    }
    snapshot["epoch"] = highest_epoch;
    snapshot["reconfig"] = reconfig;

    if (reconfig) {
        Value init;
        int highest_gid = groupId;
        string latest_members;
        int lastRound = -1;
        string longest_Qd;
        multimap<int, tuple_qc> Qc_merge;
        for (auto elem : CCSnapshots) {
            string replica_state = elem.second;
            Value jState;
            reader.parse(replica_state, jState);

            /*
             * retrieve the latest configuration: if some replica has seen a new configuration, use the new one.
             * Otherwise, use the existing one.
             */
            int replica_gid = jState["GID"].asInt();
            if (replica_gid >= highest_gid) {
                highest_gid = replica_gid;
                latest_members = jState["memberlist"].asString();
            }

            // retrieve the longest event queue
            string qd_str = jState["Qd"].asString();
            if (!qd_str.empty()) {
                map<tuple_qd, string> Qd_temp;
                util::convertStrToMap(qd_str, Qd_temp);
                tuple_qd qd_key = Qd_temp.rbegin()->first;
                int round = get<0>(qd_key);
                if (round > lastRound || lastRound < 0) {
                    lastRound = round;
                    longest_Qd = jState["Qd"].asString();
                    // prepare application initial state
                    init["initial_state"] = jState["last_state"];
                    init["app_state"] = jState["app_state"];
                }
            }

            // merge decisions
            string qc_str = jState["E"].asString();
            if (!qc_str.empty()) {
                multimap<int, tuple_qc> Qc_temp;
                util::convertStrToMap(qc_str, Qc_temp);
                for (multimap<int, tuple_qc>::iterator it = Qc_temp.begin();
                        it != Qc_temp.end(); it++) {
                    int round_temp = it->first;
                    tuple_qc event = it->second;
                    util::multimap_insert<int, tuple_qc>(Qc_merge, { round_temp,
                            event });
                }
            }
        }

        snapshot["GID"] = highest_gid;
        snapshot["memberlist"] = latest_members;
        snapshot["Qd"] = longest_Qd;
        string decisions = util::convertMapToStr(Qc_merge);
        snapshot["E"] = decisions;
        // in case of empty Qd, set initial_state to the local application state
        if (!init.isMember("initial_state")) {
            Value localState;
            reader.parse(CCSnapshots[ipAddress.get4().str()], localState);
            init["initial_state"] = localState["last_state"];
            init["app_state"] = localState["app_state"];
        }

        // prepare initialization state for new replicas
        string senderList = util::convertMapToStr(senders);
        string senderAddrList = util::convertMapToStr(clients);
        string JLlist = util::convertMapToStr(JL);
        string NcList = util::convertMapToStr(Nc);
        string NrList = util::convertMapToStr(Nr);
        string RgList = util::convertMapToStr(Rg);
        init["senders"] = senderList;
        init["senderAddrs"] = senderAddrList;
        init["JL"] = JLlist;
        init["Nc"] = NcList;
        init["Nr"] = NrList;
        init["Rg"] = RgList;
        init["starttime"] = cycleStart.dbl();

        snapshot["init"] = init;
    }

    return snapshot;
}

void NodeCtrl::handleCCEnd(CCEnd* cce) {
    UDPControlInfo *cceCtrl = check_and_cast<UDPControlInfo *>(
            cce->getControlInfo());
    IPvXAddress& srcAddr = cceCtrl->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = cce->getSenderName();
    if (survivals.count(sender) > 0) {
        string c = coordinator();
        if (c.compare(sender) == 0) {
            coord = sender;
            epoch = cce->getEpoch() + 1;
            groupId = cce->getGroupId();

            // reset consensus
            proposals.clear();
            toPropose.clear();
            proposed.clear();
            coordCand.clear();
            if (coord.compare(ipAddress.get4().str()) != 0) {
                mode = EH;
            }

            // send back ACK
            CCAck* ack = new CCAck(msg::CC_ACK);
            ack->setSenderName(fullName);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            ack->setControlInfo(udpControlInfo);
            r_transmit(ack, srcAddr);

            // reconfigure node state if needed
            if (cce->getReconfig()) {
                // state sync for any unfinished reconfiguration
                string init_str = cce->getInit();
                Reader reader;
                Value init;
                reader.parse(init_str, init);
                if (groupId < 0) {
                    // init event collection and event handling cycles for a new replica
                    simtime_t starttime = init["starttime"].asDouble();
                    cycleStart = starttime;
                    simtime_t current = simTime();
                    round = (current >= starttime) ?
                            floor((current - starttime) / roundCycle) : -1;
                    next_round_time = starttime + (round + 1) * roundCycle;
                    scheduleAt(next_round_time, ECTimeout);
//                    scheduleAt(next_round_time, updateTimeout);
                    scheduleAt(next_round_time + gcCycle, GCTimeout);
                }

                groupId = gid_temp = cce->getGroupId();

                string senderList = init["senders"].asString();
                util::convertStrToMap(senderList, senders);
                string senderAddrList = init["senderAddrs"].asString();
                util::convertStrToMap(senderAddrList, clients);
                string JLlist = init["JL"].asString();
                util::convertStrToMap(JLlist, JL);
                string NcList = init["Nc"].asString();
                util::convertStrToMap(NcList, Nc);
                string NrList = init["Nr"].asString();
                util::convertStrToMap(NrList, Nr);
                string RgList = init["Rg"].asString();
                util::convertStrToMap(RgList, Rg);
                Value initialState = init["initial_state"];
                state.first = tuple_qd(initialState["key"]["round"].asInt(),
                        initialState["key"]["clientId"].asUInt(),
                        initialState["key"]["sequence"].asInt());
                state.second = initialState["state"].asUInt();
                lastDelivered = state.first;

                // load Qd and E (i.e., decided events)
                string Qd_str = cce->getQd();
                util::convertStrToMap(Qd_str, Qd);
                string Qc_str = cce->getQc();
                util::convertStrToMap(Qc_str, Qc);
                // reset delivery round
                deliveryRound = 0;

                // load the new group configuration
                string memberlist = cce->getGroupMembers();
                map<string, int> groupMembers;
                util::convertStrToMap(memberlist, groupMembers);
                // load new group member list
                seniorities.clear();
                members.clear();
                for (map<string, int>::iterator it = groupMembers.begin();
                        it != groupMembers.end(); it++) {
                    string host = it->first;
                    int seniority = it->second;
                    members.insert(it->first);
                    if (survivals.count(host) > 0 && members.count(host) > 0) {
                        // update seniority
                        seniorities[host] = seniority + 1;
                    }
                }

                // notify membership change to senders (i.e., the clients and neighboring Meshes)
                if (coord.compare(ipAddress.get4().str()) == 0) {
                    set<string> configs;
                    for (auto elem : members) {
                        configs.insert(elem);
                    }
                    string configList = util::convertSetToStr(configs, 0);
                    set<string> eventSenderList;
                    for (auto elem : clients) {
                        string senderAddr = elem.second;
                        eventSenderList.insert(senderAddr);
                    }
                    ConfigUpdate* configeUpdate = new ConfigUpdate(
                            msg::CONFIG_UPDATE);
                    configeUpdate->setConfigs(configList.c_str());
                    configeUpdate->setSourceName(fullName);
                    configeUpdate->setLCName(LCName.c_str());
                    // message size: size of the new configuration list + size of the logical computer name
                    configeUpdate->setByteLength(
                            configList.size() + LCName.size());
                    r_multicast(configeUpdate, eventSenderList);

                    GlobalStatisticsAccess().get()->increaseReconfigurationEnd();
                }

                /*
                 * Initialize application state.
                 * Everything will only be initialized if a reconfiguration is involved in the leader election
                 */
                app->initState(state.second);
                string appState = init["app_state"].asString();
                app->unpackState(appState);
                // initialize the deliveryRound
                deliverCycleEvent();
            }

            EV << "[" << simTime() << "s] " << fullName
                      << " loaded the new coordinator" << endl;
        } else {
            EV << "[" << simTime() << "s] coordinator candidate " << fullName
                      << " is from the previous epoch" << endl;
        }
    } else {
        EV << "[" << simTime() << "s] coordinator candidate " << fullName
                  << " has failed" << endl;
    }

    delete cce;
}

void NodeCtrl::handleCCAck(CCAck* ack) {
    UDPControlInfo *ackCtrl = check_and_cast<UDPControlInfo *>(
            ack->getControlInfo());
    IPvXAddress& srcAddr = ackCtrl->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = ack->getSenderName();

    EV << "[" << simTime() << "s] " << fullName
              << " received coordinator change acknowledgement from "
              << senderName << endl;

    // only handle messages from survived members, TODO but this condition may not needed
    set<string> survived_members = survivedMembers();
    if (survived_members.count(sender) > 0) {
        CCAcks.insert(sender);
        checkCCAcks();
    }

    delete ack;
}

void NodeCtrl::checkCCAcks() {
    bool allReceived = true;

    set<string> survived_members = survivedMembers();
    // check whether all replies from  hosts on the Survivals list have been received
    for (set<string>::iterator it = survived_members.begin();
            it != survived_members.end(); ++it) {
        string hostName = *it;
        if (CCAcks.find(hostName) == CCAcks.end()) {
            allReceived = false;
            break;
        }
    }

    if (allReceived) {
        mode = EH;
        GlobalStatisticsAccess().get()->increaseLeaderElectionEnd();
    }
}

bool NodeCtrl::checkConfig() {
    bool triggerGR = false;
    bool triggerGRSilent = false;
    set<string> newMembers;
    set_difference(survivals.begin(), survivals.end(), members.begin(),
            members.end(), inserter(newMembers, newMembers.begin()));
    // Upon R \ G ¡Ù empty ¡Ä R ¡Ù G_T (see the original paper)
    if (newMembers.size() > 0 && survivals != membersTemp) {
        membersTemp = survivals;
        if (newMembers.size() > reconfigTH && reconfigTH > 0) {
            triggerGR = true;
        } else {
            triggerGRSilent = true;
        }
    }

    if (triggerGR) {
        // coordinator initiate
        if (coord.compare(ipAddress.get4().str()) == 0) {
            cout << "[" << simTime() << "s] " << fullName
                    << " start a reconfiguration" << endl;

            mode = GR;

            // in case a GR is interrupted by another GR
            gid_temp = gid_temp > groupId ? (gid_temp + 1) : (groupId + 1);
            // for interrupting another reconfiguration
            GRSnapshots.clear();

            // send G' to all the hosts on the Survivals list
            GRQuery* reconfig = new GRQuery(msg::GROUP_RECONFIG);
            reconfig->setSenderName(fullName);
            reconfig->setEpoch(epoch);
            reconfig->setGroupId(gid_temp);
            r_multicast(reconfig, membersTemp);

            GlobalStatisticsAccess().get()->increaseReconfigurationStart();
        }
        return true;
    } else if (triggerGRSilent) {

        if (coord.compare(ipAddress.get4().str()) == 0) {

            cout << "[" << simTime() << "s] " << fullName
                    << " start a restoration" << endl;

            string snapshort = buildRestoreSnapshot();
            Reader reader;
            Value j_snapshort;
            reader.parse(snapshort, j_snapshort);
            Value j_initState = j_snapshort["init"];
            FastWriter fw;
            string initState = fw.write(j_initState);
            string s_memberList = j_snapshort["G"].asCString();

            // silently G' to all the hosts on the Survivals list
            GRSilent* reconfig = new GRSilent(msg::GROUP_RECONFIG_SILENT);
            reconfig->setSenderName(fullName);
            reconfig->setEpoch(epoch);
            reconfig->setGroupId(groupId);
            reconfig->setMemberList(s_memberList.c_str());
            // message size: size of new member list + 1 byte for epoch + 1 byte for group ID
            reconfig->setBitLength(s_memberList.size() + 3);
            r_multicast(reconfig, membersTemp);

            // init the new replicas
            set<string> survivedNewMembers = survivedList(newMembers);
            for (auto elem : survivedNewMembers) {
                Restore* restore = new Restore(msg::RESTORE);
                restore->setSenderName(fullName);
                restore->setEpoch(j_snapshort["epoch"].asInt());
                restore->setGroupId(j_snapshort["GID"].asInt());
                restore->setGroupMembers(s_memberList.c_str());
                restore->setQd(j_snapshort["Qd"].asCString());
                restore->setQc(j_snapshort["E"].asCString());
                restore->setInit(initState.c_str());
                // message size: snapshot size + size of new member list + 1 byte for epoch + 1 byte for group ID
                restore->setBitLength(
                        snapshort.size() + survivedNewMembers.size() + 2);
                IPvXAddress dest = getIPAddress(elem);
                UDPControlInfo* udpControlInfo = new UDPControlInfo();
                udpControlInfo->setDestAddr(dest);
                udpControlInfo->setSrcAddr(ipAddress);
                restore->setControlInfo(udpControlInfo);
                r_transmit(restore, dest);
            }
        }
    } else {
        return false;
    }

    return false;
}

string NodeCtrl::buildRestoreSnapshot() {
    Value snapshot;
    string Qdstr = util::convertMapToStr(Qd);
    string Qcstr = util::convertMapToStr(Qc);
    snapshot["Qd"] = Qdstr;
    snapshot["E"] = Qcstr;
    snapshot["epoch"] = epoch;
    snapshot["GID"] = groupId;
    // retrieve new group members from G'
    // maps of (IPAddress, age) pair
    map<string, int> groupMembers;
    for (set<string>::iterator it = membersTemp.begin();
            it != membersTemp.end(); it++) {
        string memberAddr = *it;
        int memberAge = seniorities[memberAddr];
        groupMembers[*it] = memberAge;
    }
    string memberlist = util::convertMapToStr(groupMembers);
    snapshot["G"] = memberlist;

    Value initState;
    // prepare initialization state for new replicas
    string senderList = util::convertMapToStr(senders);
    string senderAddrList = util::convertMapToStr(clients);
    string JLlist = util::convertMapToStr(JL);
    string NcList = util::convertMapToStr(Nc);
    string NrList = util::convertMapToStr(Nr);
    string RgList = util::convertMapToStr(Rg);
    initState["senders"] = senderList;
    initState["senderAddrs"] = senderAddrList;
    initState["JL"] = JLlist;
    initState["Nc"] = NcList;
    initState["Nr"] = NrList;
    initState["Rg"] = RgList;
    initState["starttime"] = cycleStart.dbl();
    Value lastState;
    lastState["key"]["round"] = (int) get<0>(state.first);
    lastState["key"]["clientId"] = (unsigned int) get<1>(state.first);
    lastState["key"]["sequence"] = (int) get<2>(state.first);
    lastState["state"] = (unsigned int) state.second;
    initState["last_state"] = lastState;
    string appState = app->packState();
    initState["app_state"] = appState;
    string lastQState = app->packLastQState();
    initState["last_q_state"] = lastQState;
    snapshot["init"] = initState;
    FastWriter fw;

    cout << "snapshort: " << snapshot << endl;

    return fw.write(snapshot);
}

void NodeCtrl::handleGRSilent(GRSilent* gr) {
    IPvXAddress& srcAddr = check_and_cast<UDPControlInfo *>(
            gr->getControlInfo())->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = gr->getSenderName();
    int e = gr->getEpoch();
    int gid = gr->getGroupId();
    if (e == epoch && gid == groupId && mode == EH
            && coord.compare(sender) == 0) {

        // load the new group configuration
        string memberlist = gr->getMemberList();
        map<string, int> groupMembers;
        util::convertStrToMap(memberlist, groupMembers);
        // load new group member list
        seniorities.clear();
        members.clear();
        for (map<string, int>::iterator it = groupMembers.begin();
                it != groupMembers.end(); it++) {
            string host = it->first;
            int seniority = it->second;
            members.insert(it->first);
            if (survivals.count(host) > 0 && members.count(host) > 0) {
                // update seniority
                seniorities[host] = seniority + 1;
            }
        }
        membersTemp.clear();

        // notify configuration change to senders (i.e., the clients and neighboring Meshes)
        if (coord.compare(ipAddress.get4().str()) == 0) {
            set<string> configs;
            for (auto elem : members) {
                configs.insert(elem);
            }
            string configList = util::convertSetToStr(configs, 0);
            set<string> eventSenderList;
            for (auto elem : clients) {
                string senderAddr = elem.second;
                eventSenderList.insert(senderAddr);
            }
            ConfigUpdate* configeUpdate = new ConfigUpdate(msg::CONFIG_UPDATE);
            configeUpdate->setConfigs(configList.c_str());
            configeUpdate->setSourceName(fullName);
            configeUpdate->setLCName(LCName.c_str());
            // message size: size of the new configuration list + size of the logical computer name
            configeUpdate->setByteLength(configList.size() + LCName.size());
            r_multicast(configeUpdate, eventSenderList);

            GlobalStatisticsAccess().get()->increaseReconfigurationEnd();
        }
    }

    delete gr;
}

void NodeCtrl::handleRestore(Restore* restore) {
    IPvXAddress& srcAddr = check_and_cast<UDPControlInfo *>(
            restore->getControlInfo())->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = restore->getSenderName();
    int e = restore->getEpoch();
    int gid = restore->getGroupId();
    if (e >= epoch && gid >= groupId) {
        // update coordinator info for new members
        epoch = e;
        coord = sender;
        groupId = gid;

        // Sync inital states
        string init_str = restore->getInit();
        Reader reader;
        Value init;
        reader.parse(init_str, init);

        // init event collection and event handling cycles for a new replica
        simtime_t starttime = init["starttime"].asDouble();
        cycleStart = starttime;
        simtime_t current = simTime();
        round = (current >= starttime) ?
                floor((current - starttime) / roundCycle) : -1;
        next_round_time = starttime + (round + 1) * roundCycle;
        scheduleAt(next_round_time, ECTimeout);
//            scheduleAt(next_round_time, updateTimeout);
        scheduleAt(next_round_time + gcCycle, GCTimeout);

        string senderList = init["senders"].asString();
        util::convertStrToMap(senderList, senders);
        string senderAddrList = init["senderAddrs"].asString();
        util::convertStrToMap(senderAddrList, clients);
        string JLlist = init["JL"].asString();
        util::convertStrToMap(JLlist, JL);
        string NcList = init["Nc"].asString();
        util::convertStrToMap(NcList, Nc);
        string NrList = init["Nr"].asString();
        util::convertStrToMap(NrList, Nr);
        string RgList = init["Rg"].asString();
        util::convertStrToMap(RgList, Rg);
        Value lastState = init["last_state"];
        state.first = tuple_qd(lastState["key"]["round"].asInt(),
                lastState["key"]["clientId"].asUInt(),
                lastState["key"]["sequence"].asInt());
        state.second = lastState["state"].asUInt();
        lastDelivered = state.first;

        // load Qd and E (i.e., decided events)
        string Qd_str = restore->getQd();
        util::convertStrToMap(Qd_str, Qd);
        string Qc_str = restore->getQc();
        util::convertStrToMap(Qc_str, Qc);

        // load the new group configuration
        string memberlist = restore->getGroupMembers();
        map<string, int> groupMembers;
        util::convertStrToMap(memberlist, groupMembers);
        // load new group member list
        seniorities.clear();
        members.clear();
        for (map<string, int>::iterator it = groupMembers.begin();
                it != groupMembers.end(); it++) {
            string host = it->first;
            int seniority = it->second;
            members.insert(it->first);
            if (survivals.count(host) > 0 && members.count(host) > 0) {
                // update seniority
                seniorities[host] = seniority + 1;
            }
        }

        // resume event handling
        membersTemp.clear();
        mode = EH;

        cout << fullName << " appState: " << init["app_state"].asString()
                << endl;
        cout << fullName << " last_q_state: " << init["last_q_state"].asString()
                << endl;
        cout << fullName << " lastDelivered key: " << lastDelivered << endl;
        cout << fullName << " lastDelivered event: " << state.second << endl;

        // initialize application state
        app->initState(state.second);
        string appState = init["app_state"].asString();
        app->unpackState(appState);
        string lastQState = init["last_q_state"].asString();
        app->unpackLastQState(lastQState);

        // initialize the deliveryRound
        deliverCycleEvent();

        EV << "[" << simTime() << "s] " << fullName
                  << " loaded the new configuration" << endl;
    }

    delete restore;
}

void NodeCtrl::handleGRQuery(GRQuery* gr) {
    IPvXAddress& srcAddr = check_and_cast<UDPControlInfo *>(
            gr->getControlInfo())->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = gr->getSenderName();
    int e = gr->getEpoch();
    int gid = gr->getGroupId();

    EV << "[" << simTime() << "s] " << fullName
              << " received reconfiguration query from " << senderName << endl;

    // message validation
    if (e >= epoch && gid >= groupId && mode != CC) {

        mode = GR;
        EV << "[" << simTime() << "s] " << fullName
                  << " entered the reconfiguration mode" << endl;

        gid_temp = gid;

        // update coordinator info for new members
        if (groupId < 0) {
            epoch = e;
            coord = sender;
        }

        GRQueryReply* grr = new GRQueryReply(msg::RECONFIG_READY);
        grr->setSenderName(fullName);
        grr->setEpoch(epoch);
        grr->setGroupId(gid_temp);
        // send events for state sync
        string snapshot = buildGRSnapshot();
        grr->setState(snapshot.c_str());
        // message size: snapshot size + 1 byte for epoch + 1 byte for group ID
        grr->setByteLength(snapshot.size() + 2);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        grr->setControlInfo(udpControlInfo);

        r_transmit(grr, srcAddr);
    } else {
        // in case of GID competition, or that the last leader election end messae has not been received
        GRNAck* nack = new GRNAck(msg::GR_NACK);
        nack->setSenderName(fullName);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        nack->setControlInfo(udpControlInfo);
        r_transmit(nack, srcAddr);
    }

    delete gr;
}

string NodeCtrl::buildGRSnapshot() {
    Value snapshot;
    string Qdstr = util::convertMapToStr(Qd);
    string Qcstr = util::convertMapToStr(Qc);
    snapshot["Qd"] = Qdstr;
    snapshot["E"] = Qcstr;

    Value lastState;
    lastState["key"]["round"] = (int) get<0>(state.first);
    lastState["key"]["clientId"] = (unsigned int) get<1>(state.first);
    lastState["key"]["sequence"] = (int) get<2>(state.first);
    lastState["state"] = (unsigned int) state.second;
    snapshot["last_state"] = lastState;
    string appState = app->packState();
    snapshot["app_state"] = appState;
    FastWriter fw;
    return fw.write(snapshot);
}

void NodeCtrl::handleGRNAck(GRNAck* grnack) {
    UDPControlInfo *grnackCtrl = check_and_cast<UDPControlInfo *>(
            grnack->getControlInfo());
    IPvXAddress& srcAddr = grnackCtrl->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = grnack->getSenderName();
    // restart the reconfiguration process with a higher temporary GID
    if (survivals.count(sender)) {
        membersTemp.clear();
        int replica_gid = grnack->getGidTemp();
        gid_temp = replica_gid > gid_temp ? (replica_gid + 1) : (gid_temp + 1);
    }

    delete grnack;
}

void NodeCtrl::handleGRQueryReply(GRQueryReply* grr) {
    IPvXAddress& srcAddr = check_and_cast<UDPControlInfo *>(
            grr->getControlInfo())->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = grr->getSenderName();
    int e = grr->getEpoch();
    int gid = grr->getGroupId();

    EV << "[" << simTime() << "s] " << fullName
              << " received group reconfiguration reply from " << senderName
              << endl;

    // only handle messages from survived hosts in neighborsTemp
    if (e == epoch && gid == gid_temp && mode == GR
            && coord.compare(ipAddress.get4().str()) == 0
            && membersTemp.count(sender) > 0 && survivals.count(sender) > 0) {
        GRSnapshots[sender] = grr->getState();
        checkGRSnapshots();
    }

    delete grr;
}

void NodeCtrl::checkGRSnapshots() {
    if (mode != CC) {
        bool allReceived = true;
        set<string> intersection;
        set_intersection(survivals.begin(), survivals.end(),
                membersTemp.begin(), membersTemp.end(),
                inserter(intersection, intersection.begin()));
        // check whether all replies from  hosts on the Survivals list have been received
        for (set<string>::iterator it = intersection.begin();
                it != intersection.end(); ++it) {
            string host = *it;
            if (GRSnapshots.count(host) == 0) {
                allReceived = false;
                break;
            }
        }

        if (allReceived) {
            Value snapshot = computeGRSnapshot();

            if (hasGUI()) {
//                cout << "[" << simTime() << "][" << fullName << "] snapshot: "
//                        << snapshot << endl;
            }

            GREnd* gre = new GREnd(msg::LOAD_CONFIG);
            gre->setSenderName(fullName);
            gre->setEpoch(epoch);
            gre->setGroupId(gid_temp);
            gre->setQd(snapshot["Qd"].asCString());
            gre->setQc(snapshot["E"].asCString());
            FastWriter fw;
            string s_snapshort = fw.write(snapshot["init"]);
            gre->setInit(s_snapshort.c_str());
            gre->setGroupMembers(snapshot["memberlist"].asCString());
            // message size: 1 byte for epoch + 1 byte for group Id + snapshot size
            gre->setByteLength(1 + 1 + s_snapshort.size());
            r_multicast(gre, intersection);
        }
    }
}

Value NodeCtrl::computeGRSnapshot() {
    Value snapshot;
    Value init;
    string longest_Qd;
    int lastRound = -1;
    multimap<int, tuple_qc> Qc_merge;
    Reader reader;
    for (map<string, string>::iterator it = GRSnapshots.begin();
            it != GRSnapshots.end(); it++) {
        string replica = it->first;
        string replica_state = it->second;
        Value jstate;
        reader.parse(replica_state, jstate);

        if (hasGUI()) {
//            cout << "[" << simTime() << "][" << fullName
//                    << "]  state of replica [" << replica << "]: " << jstate
//                    << endl;
        }

        // retrieve the longest event queue
        string qd_str = jstate["Qd"].asString();
        if (!qd_str.empty()) {
            map<tuple_qd, string> Qd_temp;
            util::convertStrToMap(qd_str, Qd_temp);
            tuple_qd qd_key = Qd_temp.rbegin()->first;
            int round = get<0>(qd_key);
            if (round > lastRound || lastRound < 0) {
                lastRound = round;
                longest_Qd = jstate["Qd"].asString();
                // prepare application initial state
                init["initial_state"] = jstate["last_state"];
                init["app_state"] = jstate["app_state"];
            }
        }

        // merge decisions
        string qc_str = jstate["E"].asString();
        if (!qc_str.empty()) {
            multimap<int, tuple_qc> Qc_temp;
            util::convertStrToMap(qc_str, Qc_temp);
            for (multimap<int, tuple_qc>::iterator it = Qc_temp.begin();
                    it != Qc_temp.end(); it++) {
                int round_temp = it->first;
                tuple_qc event = it->second;
                util::multimap_insert<int, tuple_qc>(Qc_merge, { round_temp,
                        event });
            }
        }
    }

    string decisions = util::convertMapToStr(Qc_merge);
    snapshot["E"] = decisions;
    snapshot["Qd"] = longest_Qd;
    // in case of empty Qd, set initial_state to the local application state
    if (!init.isMember("initial_state")) {
        Value localState;
        reader.parse(GRSnapshots[ipAddress.get4().str()], localState);
        init["initial_state"] = localState["last_state"];
        init["app_state"] = localState["app_state"];
    }

    // prepare initialization state for new replicas
    string senderList = util::convertMapToStr(senders);
    string senderAddrList = util::convertMapToStr(clients);
    string JLlist = util::convertMapToStr(JL);
    string NcList = util::convertMapToStr(Nc);
    string NrList = util::convertMapToStr(Nr);
    string RgList = util::convertMapToStr(Rg);
    init["senders"] = senderList;
    init["senderAddrs"] = senderAddrList;
    init["JL"] = JLlist;
    init["Nc"] = NcList;
    init["Nr"] = NrList;
    init["Rg"] = RgList;
    init["starttime"] = cycleStart.dbl();
    snapshot["init"] = init;

    // retrieve new group members from G'
    // maps of (IPAddress, age) pair
    map<string, int> groupMembers;
    for (set<string>::iterator it = membersTemp.begin();
            it != membersTemp.end(); it++) {
        string memberAddr = *it;
        int memberAge = seniorities[memberAddr];
        groupMembers[*it] = memberAge;
    }
    string memberlist = util::convertMapToStr(groupMembers);
    snapshot["memberlist"] = memberlist;

    return snapshot;
}

void NodeCtrl::handleGREnd(GREnd* gre) {
    IPvXAddress& srcAddr = check_and_cast<UDPControlInfo *>(
            gre->getControlInfo())->getSrcAddr();
    string sender = srcAddr.get4().str();
    string senderName = gre->getSenderName();
    int e = gre->getEpoch();
    int gid = gre->getGroupId();
//    if (e == epoch && gid == groupId && mode == GR
//            && coord.compare(sender) == 0) {
    if (e == epoch && gid == gid_temp && mode == GR
            && coord.compare(sender) == 0) {
        // Sync inital states
        string init_str = gre->getInit();
        Reader reader;
        Value init;
        reader.parse(init_str, init);
        if (groupId < 0) {
            // init event collection and event handling cycles for a new replica
            simtime_t starttime = init["starttime"].asDouble();
            cycleStart = starttime;
            simtime_t current = simTime();
            round = (current >= starttime) ?
                    floor((current - starttime) / roundCycle) : -1;
            next_round_time = starttime + (round + 1) * roundCycle;
            scheduleAt(next_round_time, ECTimeout);
//            scheduleAt(next_round_time, updateTimeout);
            scheduleAt(next_round_time + gcCycle, GCTimeout);
        }

        groupId = gid_temp;

        string senderList = init["senders"].asString();
        util::convertStrToMap(senderList, senders);
        string senderAddrList = init["senderAddrs"].asString();
        util::convertStrToMap(senderAddrList, clients);
        string JLlist = init["JL"].asString();
        util::convertStrToMap(JLlist, JL);
        string NcList = init["Nc"].asString();
        util::convertStrToMap(NcList, Nc);
        string NrList = init["Nr"].asString();
        util::convertStrToMap(NrList, Nr);
        string RgList = init["Rg"].asString();
        util::convertStrToMap(RgList, Rg);
        Value initialState = init["initial_state"];
        state.first = tuple_qd(initialState["key"]["round"].asInt(),
                initialState["key"]["clientId"].asUInt(),
                initialState["key"]["sequence"].asInt());
        state.second = initialState["state"].asUInt();
        lastDelivered = state.first;

        // load Qd and E (i.e., decided events)
        string Qd_str = gre->getQd();
        util::convertStrToMap(Qd_str, Qd);
        string Qc_str = gre->getQc();
        util::convertStrToMap(Qc_str, Qc);
        // reset delivery round
        deliveryRound = 0;
        // reset consensus
        proposals.clear();
        toPropose.clear();
        proposed.clear();

        // load the new group configuration
        string memberlist = gre->getGroupMembers();
        map<string, int> groupMembers;
        util::convertStrToMap(memberlist, groupMembers);
        // load new group member list
        seniorities.clear();
        members.clear();
        for (map<string, int>::iterator it = groupMembers.begin();
                it != groupMembers.end(); it++) {
            string host = it->first;
            int seniority = it->second;
            members.insert(it->first);
            if (survivals.count(host) > 0 && members.count(host) > 0) {
                // update seniority
                seniorities[host] = seniority + 1;
            }
        }

        // resume event handling
        membersTemp.clear();
        mode = EH;

        // notify membership change to senders (i.e., the clients and neighboring Meshes)
        if (coord.compare(ipAddress.get4().str()) == 0) {
            set<string> configs;
            for (auto elem : members) {
                configs.insert(elem);
            }
            string configList = util::convertSetToStr(configs, 0);
            set<string> eventSenderList;
            for (auto elem : clients) {
                string senderAddr = elem.second;
                eventSenderList.insert(senderAddr);
            }
            ConfigUpdate* configeUpdate = new ConfigUpdate(msg::CONFIG_UPDATE);
            configeUpdate->setConfigs(configList.c_str());
            configeUpdate->setSourceName(fullName);
            configeUpdate->setLCName(LCName.c_str());
            // message size: size of the new configuration list + size of the logical computer name
            configeUpdate->setByteLength(configList.size() + LCName.size());
            r_multicast(configeUpdate, eventSenderList);

            GlobalStatisticsAccess().get()->increaseReconfigurationEnd();
        }

        // initialize application state
        app->initState(state.second);
        string appState = init["app_state"].asString();
        app->unpackState(appState);
        // initialize the deliveryRound
        deliverCycleEvent();

        EV << "[" << simTime() << "s] " << fullName
                  << " loaded the new configuration" << endl;
    } else {
        EV << "[" << simTime() << "s] reconfiguration from " << fullName
                  << " is invalid" << endl;
    }

    delete gre;
}

set<string> NodeCtrl::survivedMembers() {
    set<string> survived_members = util::intersectSets(survivals, members);
    return survived_members;
}

set<string> NodeCtrl::survivedList(set<string> membList) {
    set<string> survived_list = util::intersectSets(survivals, membList);
    return survived_list;
}

//simtime_t NodeCtrl::readClock() {
//    Clock* clock = check_and_cast<Clock*>(
//            getParentModule()->getSubmodule("clock"));
//    clockError = clock->getError();
//    simtime_t read = HostBase::readClock();
//    return read;
//}
