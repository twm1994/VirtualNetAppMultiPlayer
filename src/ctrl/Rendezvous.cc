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

#include "Rendezvous.h"

Define_Module(Rendezvous);

Rendezvous::Rendezvous() {
    hostCreated = 0;
    STAGE_NUM = stage::RENDEZVOUS_INIT;
}

Rendezvous::~Rendezvous() {
    disposeTimeouts();
}

int Rendezvous::numInitStages() const {
    return STAGE_NUM;
}

void Rendezvous::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();

        cycle = par("cycle");

        configurator = UnderlayConfiguratorAccess().get();

        WATCH(hostCreated);
        WATCH_SET(survivals);
        WATCH_SET(toTerminates);
    }

    if (stage == 1) {
//        // initialize replica group
//        // add new hosts at the beginning
//        int groupSize =
//                ReplicaNumPolicyAccess().get(this->getParentModule())->maxReplicNumber();
//        addNodes(groupSize, true);
    }
}

void Rendezvous::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::HEARTBEAT_RESPONSE)) {
        HBResponse* hbr = check_and_cast<HBResponse*>(msg);
        handleHeartbeatResp(hbr);
    } else if (msg->isName(msg::HEARTBEAT_TIMEOUT)) {
        HBTimeout* timeout = check_and_cast<HBTimeout*>(msg);
        handleTimeout(timeout);
    } else if (msg->isName(msg::REQ_CONNECT)
            || msg->isName(msg::REQ_CONNECT_NEIGHBOR)) {
        Connect* connect = check_and_cast<Connect*>(msg);
        handleClientConnect(connect);
    } else if (msg->isName(msg::TERMINATION)) {
        handleTermination(msg);
    }
}

void Rendezvous::handleTermination(cMessage* msg) {
    UDPControlInfo* msgCtrl = check_and_cast<UDPControlInfo *>(
            msg->getControlInfo());
    IPvXAddress& srcAddr = msgCtrl->getSrcAddr();
    toTerminates.insert(srcAddr);

    // check whether the termination signal has been received from all live replies
    bool allReceived = true;
    for (set<IPvXAddress>::iterator it = survivals.begin();
            it != survivals.end(); ++it) {
        IPvXAddress host = *it;
        if (toTerminates.find(host) == toTerminates.end()) {
            allReceived = false;
            break;
        }
    }

    cout << "Termination received" << endl;

    if (allReceived) {

        cout << "Terminate the Logical Computer" << endl;

        for (auto elem : survivals) {
//            configurator->removeNode(elem);
            configurator->revokeNode(elem);
        }
        configurator->revokeLogicComputer(ipAddress);
        toExit = true;
    }
    delete msg;
}

void Rendezvous::handleClientConnect(Connect* msg) {
    string senderName = msg->getSenderModule()->getFullName();
    UDPControlInfo* msgCtrl = check_and_cast<UDPControlInfo *>(
            msg->getControlInfo());
    IPvXAddress& srcAddr = msgCtrl->getSrcAddr();

    if (msg->isName(msg::REQ_CONNECT)) {
        LCName = msg->getLCName();
        peer_id = msg->getPeerId();
    }

    // in case the logical computer name is unknown, the neighbor connection request will be discarded
    if (msg->isName(msg::REQ_CONNECT)
            || (msg->isName(msg::REQ_CONNECT_NEIGHBOR) && !LCName.empty())) {

        // initialize replica group
        // add new hosts at the beginning
        int groupSize =
                ReplicaNumPolicyAccess().get(this->getParentModule())->maxReplicNumber();
        addNodes(groupSize, true);

        string msgName =
                msg->isName(msg::REQ_CONNECT) ?
                        msg::REPLY_CONNECT : msg::REPLY_CONNECT_NEIGHBOR;
        ConnectReply* reply = new ConnectReply(msgName.c_str());
        reply->setLCName(LCName.c_str());
        stringstream sstream;
        for (set<IPvXAddress>::iterator it = survivals.begin();
                it != survivals.end(); ++it) {
            IPvXAddress address = *it;
            string addrStr = address.get4().str();
            sstream << addrStr << ";";
        }
        string addrs = sstream.str();
        reply->setHostAddrs(addrs.c_str());
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(srcAddr);
        udpControlInfo->setSrcAddr(ipAddress);
        reply->setControlInfo(udpControlInfo);
        sendIdeal(reply);
    }
    delete msg;
}

void Rendezvous::handleHeartbeatResp(HBResponse* hbr) {

    string host = hbr->getSourceName();

    if (timeouts.count(host) > 0) {
        TimeoutScheduler* ts = timeouts[host];
        if (ts->hbr != NULL) {
            delete ts->hbr;
        }
        hbr->setTimestamp(simTime());
        ts->hbr = hbr;
    }
}

void Rendezvous::handleTimeout(HBTimeout* hbt) {
    string host = hbt->getHostName();
    TimeoutScheduler* ts = timeouts[host];
    simtime_t receiveTime = 0;
    if (ts->hbr != NULL) {
        receiveTime = ts->hbr->getTimestamp();
    }
    simtime_t diff = simTime() - receiveTime;

    if (diff > cycle) {
        cout << "[" << simTime() << "s] " << "timeout: from " << host << " to "
                << fullName << endl;

        string hostAddr = hbt->getHostAddr();
        TimeoutScheduler* timer = timeouts[host];
        cout << "[" << simTime() << "s] " << "Rendezvous remove event host: "
                << timer->event->getHostName() << endl;

        if (hasGUI()) {
            string msg = "Remove " + host + " from group";
            bubble(msg.c_str());
        }

        IPvXAddress addr(hostAddr.c_str());
        survivals.erase(addr);
        routingTable.erase(addr);
        timeouts.erase(host);

        // add new hosts if the current number of hosts is lower than the threshold
        // get the logical computer module
        //        int threshold =
        //                ReplicaNumPolicyAccess().get(this->getParentModule())->getThreshold();
        int groupSize =
                ReplicaNumPolicyAccess().get(this->getParentModule())->maxReplicNumber();
        int size = survivals.size();
        //        if (size < threshold) {
        // if, in a small chance, all replica has been failed, re-initialized the entire replica group
        bool init = (size == 0 ? true : false);
        this->addNodes(groupSize - size, init);
        //        }

        if (timer->hbr != NULL) {
            delete timer->hbr;
        }
        cancelAndDelete(timer->event);

    } else {
        if (timeouts.find(host) != timeouts.end()) {
            TimeoutScheduler* timer = timeouts[host];
            timer->timeout = simTime() + cycle;

            HBProbe *heartbeat = new HBProbe(msg::HEARTBEAT);
            stringstream sstream;
            for (map<IPvXAddress, string>::iterator it = routingTable.begin();
                    it != routingTable.end(); ++it) {
                string moduleName = it->first.get4().str();
                sstream << moduleName << ";";
            }
            string members = sstream.str();
            heartbeat->setSurvivals(members.c_str());

            //        int gateId = group[host];
            heartbeat->setSourceName(fullName);
            heartbeat->setDestName(host.c_str());
            heartbeat->setTimestamp(simTime());
            heartbeat->setLCName(LCName.c_str());
            heartbeat->setIsInit(false);

            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            IPvXAddress srcAddr(hbt->getHostAddr());
            udpControlInfo->setDestAddr(srcAddr);
            udpControlInfo->setSrcAddr(ipAddress);
            heartbeat->setControlInfo(udpControlInfo);
            sendFixedDelay(heartbeat);

            // reset timer
            timer->event->setTimeout(timer->timeout);
            scheduleAt(timer->timeout, timer->event);
            // update the monitored timeout value
            timeouts[host] = timer;
        } else {
            cout << "[" << simTime() << "s] " << "Survivals no longer contain "
                    << host << endl;
        }
    }
}

void Rendezvous::addNodes(int num, bool isInit) {
    EV << "[" << simTime() << "s] create " << num << " new hosts" << endl;
    vector<IPvXAddress> newHosts;
    cModule* LC = getParentModule()->getParentModule();
    simtime_t starttime = par("starttime") + simTime();

    for (int i = 0; i < num; i++) {
        TransportAddress* nodeAddr = configurator->createNode(LC, hostCreated);
        string nodeName = LC->getSubmodule("node", hostCreated)->getFullName();
        hostCreated++;
        routingTable[nodeAddr->getIp()] = nodeName;
        survivals.insert(nodeAddr->getIp());
        newHosts.push_back(nodeAddr->getIp());
    }

    // initialize heartbeat
    stringstream sstream;
    for (map<IPvXAddress, string>::iterator it = routingTable.begin();
            it != routingTable.end(); ++it) {
        string moduleName = it->first.get4().str();
        sstream << moduleName << ";";
    }
    string members = sstream.str();

    for (size_t i = 0; i < newHosts.size(); i++) {
        IPvXAddress& ip = newHosts[i];
        string hostName = routingTable[ip];

        HBProbe *heartbeat = new HBProbe(msg::HEARTBEAT);
        heartbeat->setSurvivals(members.c_str());
        heartbeat->setSourceName(fullName);
        heartbeat->setDestName(hostName.c_str());
        heartbeat->setTimestamp(simTime());
        heartbeat->setLCName(LCName.c_str());
        heartbeat->setPeerId(peer_id);
        heartbeat->setIsInit(isInit);
        if (isInit) {
            heartbeat->setStartTime(starttime);
//            heartbeat->setThreshold(0);
        } else {
            int threshold = ReplicaNumPolicyAccess().get(
                    this->getParentModule())->getThreshold();
            heartbeat->setThreshold(threshold);
        }
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(ip);
        udpControlInfo->setSrcAddr(ipAddress);
        heartbeat->setControlInfo(udpControlInfo);
        sendFixedDelay(heartbeat);

        // set heartbeat timeout
        TimeoutScheduler* te = new TimeoutScheduler();
        te->timeout = simTime() + cycle;
        HBTimeout* to = new HBTimeout(msg::HEARTBEAT_TIMEOUT);
        to->setHostAddr(ip.get4().str().c_str());
        to->setHostName(hostName.c_str());
        to->setTimeout(te->timeout);
        te->event = to;
        EV << "[" << simTime() << "s] " << hostName << " join the group"
                  << endl;
        timeouts[hostName] = te;
        scheduleAt(te->timeout, te->event);
    }

    newHosts.clear();
}

void Rendezvous::disposeTimeouts() {
    try {
        for (map<string, TimeoutScheduler*>::iterator it = timeouts.begin();
                it != timeouts.end(); ++it) {
            TimeoutScheduler* timer = it->second;
            if (timer != NULL) {
                if (timer->event != NULL) {
                    cancelAndDelete(timer->event);

                }
                if (timer->hbr != NULL) {
                    delete timer->hbr;
                }
                delete timer;
            }
        }
        timeouts.clear();
    } catch (exception& e) {
        EV << e.what() << endl;
    }
}
