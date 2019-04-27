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

#include "ClientCtrl.h"
#include "../app/ClientApp.h"

Define_Module(ClientCtrl);
std::ofstream dfile_cc("VirtualNet_ClientCtrl.txt");
std::ostream dout_cc(dfile_cc.rdbuf());

ClientCtrl::ClientCtrl() {
    STAGE_NUM = stage::CLIENT_INIT;
    id = 0;

    loadTimeout = new cMessage(msg::LOAD_TIMEOUT);
    neighborDiscovery = new cMessage(msg::NEIGHBOR_DISCOVERY);
}

ClientCtrl::~ClientCtrl() {
    if (loadTimeout != NULL) {
        cancelAndDelete(loadTimeout);
    }
    if (neighborDiscovery != NULL) {
        cancelAndDelete(neighborDiscovery);
    }

    disposeTimeouts();
}

void ClientCtrl::disposeTimeouts() {
    try {
        for (std::map<std::string, ClientCycleTimeout*>::iterator it =
                timeouts.begin(); it != timeouts.end(); ++it) {
            ClientCycleTimeout* timer = it->second;
            if (timer != NULL) {
                cancelAndDelete(timer);
            }
        }
        timeouts.clear();
    } catch (exception& e) {
        EV << e.what() << endl;
    }
}

int ClientCtrl::numInitStages() const {
    return STAGE_NUM;
}

void ClientCtrl::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();

        // retrieve parameters and initialize timers
        loadCycle = par("loadCycle");
        connectCycle = par("initJoinTime");
        discovery = par("discoveryCycle");
        handshakeCycle = par("handshakeTime");

        // initialize variable observers
        WATCH(cycle);
        WATCH_MAP(starts);
        WATCH_MAP(trace);
        WATCH_MAP(seq);
        WATCH(id);
        WATCH_MULTIMAP(replicaMaps);
        WATCH_MULTIMAP(neighborMaps);
        WATCH_MAP(rendezvousMaps);
        WATCH_SET(neighborsToAdd);
        WATCH_SET(neighborsToRemove);
        WATCH(LCName);
    }

    if (stage == 1) {
        // refer to the upper layer app
        app = check_and_cast<ClientApp*>(
                getParentModule()->getSubmodule("app"));

        // initialize neighbor discovery
//        scheduleAt(simTime() + discovery, neighborDiscovery);

        TransportAddress* addr =
                UnderlayConfiguratorAccess().get()->registerEndpoint(
                        getParentModule());
        ipAddress = addr->getIp();
        // start checking connection
        flag = par("connect");

        scheduleAt(simTime() + loadCycle, loadTimeout);

        interactionLatency = registerSignal("latency");
    }
}

void ClientCtrl::dispatchHandler(cMessage *msg) {

    if (msg->isName(msg::JOIN)) {
        handleJoin(msg);
    } else if (msg->isName(msg::EVENT_CYCLE)) {
        handleCycleEvent(msg);
    } else if (msg->isName(msg::UPDATE)) {
        handleUpdate(msg);
    } else if (msg->isName(msg::CONFIG_UPDATE)) {
        this->handleConfigUpdate(msg);
    } else if (msg->isName(msg::LOAD_TIMEOUT)) {
        handleLoad();
    } else if (msg->isName(msg::CONNECT_TIMEOUT)) {
        handleConnectTimeout(msg);
    } else if (msg->isName(msg::REPLY_CONNECT)
            || msg->isName(msg::REPLY_CONNECT_NEIGHBOR)) {
        handleConnectReply(msg);
    } else if (msg->isName(msg::NEIGHBOR_DISCOVERY)) {
        handleNeighborDiscovery(msg);
    } else if (msg->isName(msg::NEIGHBOR_ADD)) {
        handleAddNeighbor(msg);
    } else if (msg->isName(msg::NEIGHBOR_RM)) {
        handleRMNeighbor(msg);
    } else if (msg->isName(msg::HANDSHAKE_TIMEOUT)) {
        handleHandShakeTimeout(msg);
    } else if (msg->isName(msg::HANDSHAKE_REJECT)) {
        handleHandShakeReject(msg);
    }
}

void ClientCtrl::handleAddNeighbor(cMessage* msg) {

    // for neighbor join during the Mesh init phase
    if (this->LCName.empty()) {
        delete msg;
        return;
    }

    AddNeighbor* addNeighbor = check_and_cast<AddNeighbor*>(msg);

    std::string LCName = addNeighbor->getNeighbor();
    std::string hostAddrs = addNeighbor->getHostAddrs();

    // to stop the attempt of connection once the connection has already been set up
    if (neighborsToAdd.count(LCName) == 0) {
        if (!hostAddrs.empty()) {
            vector<std::string> toAdd;
            util::splitString(hostAddrs, ";", toAdd);
            for (size_t i = 0; i < toAdd.size(); i++) {
                std::string addrStr = toAdd[i];
                IPvXAddress addr(addrStr.c_str());
                util::multimap_insert(neighborMaps, { LCName, addr });
            }
            starts[LCName] = -1;
            seq[LCName] = 0;
            neighborsToAdd.insert(LCName);
            neighborsToRemove.erase(LCName);
            // create a new application event queue for the neighbor communication
            appDataQueue[LCName];
        }

        HandShakeTimeout* hsTimeout = new HandShakeTimeout(
                msg::HANDSHAKE_TIMEOUT);
        hsTimeout->setLCName(LCName.c_str());
        // in case of re-connect requested from nodes due to cycle un-started
        cancelEvent(hsTimeout);
        scheduleAt(simTime() + connectCycle, hsTimeout);

        dout_cc << "Time to connect to neighbor" << endl;
    } else {
//        cout << "Connecting to neighbor has been timed" << endl;
    }

    delete addNeighbor;
}

void ClientCtrl::handleRMNeighbor(cMessage* msg) {

    // for neighbor leave during the Mesh init phase
    if (this->LCName.empty()) {
        delete msg;
        return;
    }

    RemoveNeighbor* rmNeighbor = check_and_cast<RemoveNeighbor*>(msg);

    std::string LCName = rmNeighbor->getNeighbor();
    // currently not used
    std::string hostAddrs = rmNeighbor->getHostAddrs();

    if (neighborsToRemove.count(LCName) == 0) {

//        cout << "To remove client: " << LCName << endl;

        neighborsToRemove.insert(LCName);
        starts.erase(LCName);
    }

    delete rmNeighbor;
}

void ClientCtrl::cacheAppData(std::string userData) {
    // append client ID
    appDataQueue[LCName].push(userData + "+" + to_string(id));
    // broadcast exit
    if (userData.find(event::TO_EXIT) < userData.length()) {
        for (auto elem : appDataQueue) {

            dout_cc << "Queue name: " << elem.first << endl;

            if (elem.first.compare(LCName) != 0) {
//                elem.second.push(userData);
                appDataQueue[elem.first].push(userData);
            }
        }
    }
}

void ClientCtrl::handleNeighborDiscovery(cMessage* msg) {

    Coordinator* coordinator = CoordinatorAccess().get();
    ClientInfo& info = coordinator->getClientInfo(fullName);
    set<ClientInfo> neighbors = coordinator->getNeighbors(fullName, info.getX(),
            info.getY());
    for (set<ClientInfo>::iterator it = neighbors.begin();
            it != neighbors.end(); it++) {
        ClientInfo neighbor = *it;

        if (!neighbor.getRendezvous().isUnspecified()) {
            std::string name = neighbor.getName();
            if (rendezvousMaps.count(name) == 0) {
                IPvXAddress neighborRendz = neighbor.getRendezvous();

                // connect to the neighbor logical computer
                Connect* connect = new Connect(msg::REQ_CONNECT_NEIGHBOR);
                UDPControlInfo* udpControlInfo = new UDPControlInfo();
                udpControlInfo->setSrcAddr(ipAddress);
                udpControlInfo->setDestAddr(neighborRendz);
                connect->setControlInfo(udpControlInfo);
                sendIdeal(connect);
            }
        }
    }

    scheduleAt(simTime() + discovery, neighborDiscovery);
}

void ClientCtrl::handleJoin(cMessage* msg) {
    Join* join = check_and_cast<Join*>(msg);
    std::string LCName = join->getLCName();
    if (starts.count(LCName) > 0 && starts[LCName] < 0) {
        cycle = join->getCycleLength();
        if (join->getJoinTime() < simTime()) {
            // calculate the time of the next cycle from the given start for first event scheduling
            double current = simTime().dbl();
            double joinTime = join->getJoinTime().dbl();
            double cycles = floor((current - joinTime) / cycle.dbl()) + 1.0;
            // start is the length of interval from the current time
            starts[LCName] = cycles * cycle.dbl() + joinTime;
            EV << "[debug] current time is: " << current << endl;
            EV << "[debug] joinTime time is: " << joinTime << endl;
            EV << "[debug] cycle time is: " << cycle << endl;
            EV << "[debug] cycles time is: " << cycles << endl;
            EV << "[debug] start time is: " << starts[LCName] << endl;
            // calculate the sequence number
            seq[LCName] += 1 + static_cast<int>(cycles);
        } else {
            // start is the length of interval from the current time
            starts[LCName] = join->getJoinTime();
            seq[LCName]++;
            EV << "[debug] start time is: " << starts[LCName] << endl;
            EV << "[debug] join sequence is: " << seq[LCName] << endl;
        }
        ClientCycleTimeout* cycleTimeout = new ClientCycleTimeout(
                msg::EVENT_CYCLE);
        cycleTimeout->setLCName(LCName.c_str());
        timeouts[LCName] = cycleTimeout;
        scheduleAt(starts[LCName], cycleTimeout);
    }

    delete join;
}

void ClientCtrl::handleCycleEvent(cMessage* msg) {
    ClientCycleTimeout* cycleTimeout = check_and_cast<ClientCycleTimeout*>(msg);
    std::string LCName = cycleTimeout->getLCName();

//    dout_cc << fullName << " send event to Mesh " << LCName << endl;
//    dout_cc << "is the queue empty? " << appDataQueue[LCName].empty() << endl;

    Event* em;
    em = new Event(msg::EVENT);
    em->setSenderId(id);
    stringstream ss;
//    ss << "User operation " << seq[LCName] << ": " << << " from " << fullName;

    std::string event = event::NO_INPUT;
    if (neighborsToRemove.count(LCName) > 0) {
        unsigned short peerId = app->getPeerId();
        event = std::string(event::TO_REMOVE) + "+" + to_string(peerId) + "+"
                + fullName + "+" + to_string(id);

//        dout_cc << fullName << "neighborsToRemove event: " << event << endl;

    } else if (!appDataQueue[LCName].empty()) {
        event = appDataQueue[LCName].front();
        appDataQueue[LCName].pop();

//        dout_cc << fullName << "client data: " << event << endl;

        if (event.find(event::TO_EXIT) < event.length()) {
            appDataQueue.erase(LCName);

            // send TO_REMOVE to other Meshes at exit
            if (LCName.compare(this->LCName) != 0) {
                unsigned short peerId = app->getPeerId();
                event = std::string(event::TO_REMOVE) + "+" + to_string(peerId)
                        + "+" + fullName + "+" + to_string(id);

//                dout_cc << fullName << "exit to neighbor event: " << event
//                        << endl;
            }

            if (appDataQueue.empty()) {
                toExit = true;
                UnderlayConfiguratorAccess().get()->revokeClient(ipAddress);
            }
        }
    }

    em->setContent(event.c_str());
    // sourceName and destName must be set for module purge
    em->setSourceName(fullName);
    em->setSeq(seq[LCName]);
    // message size: event size + 1 byte for event sequence number
    em->setByteLength(event.size() + 1);

    // broadcast the message to all live nodes
    set<IPvXAddress> replicas;
    pair<multimap<std::string, IPvXAddress>::iterator,
            multimap<std::string, IPvXAddress>::iterator> ret;
    if (LCName.compare(this->LCName) == 0) {
        ret = replicaMaps.equal_range(LCName);
    } else {
        ret = neighborMaps.equal_range(LCName);
    }

    for (multimap<std::string, IPvXAddress>::iterator iter = ret.first;
            iter != ret.second; iter++) {
        replicas.insert(iter->second);
    }
    multicast(em, replicas);
    // record event sending time
    boost::tuple<unsigned long, std::string, int> event_key(id, LCName,
            seq[LCName]);
    sentEvents[event_key] = simTime();
    // for event delivery rate calculation
    GlobalStatisticsAccess().get()->addSentEvent(fullName, LCName, seq[LCName],
            simTime());

    // Set the timer for sending the next event
    if (neighborsToRemove.count(LCName) > 0) {
        neighborMaps.erase(LCName);
        neighborsToRemove.erase(LCName);
        neighborsToAdd.erase(LCName);
        seq.erase(LCName);
        delete cycleTimeout;
        timeouts.erase(LCName);
    } else {
        bool deviate = par("deviate").boolValue();
        if (deviate) {
            // for changing the event arrival delay
            if (seq[LCName] == 3) {
                scheduleAt(simTime() + 2.1 * cycle, cycleTimeout);
            } else {
                scheduleAt(simTime() + cycle, cycleTimeout);
            }
        } else {
            scheduleAt(simTime() + cycle, cycleTimeout);
        }

        // increase event sequence number
        seq[LCName]++;
    }
}

void ClientCtrl::handleUpdate(cMessage* msg) {

    Update* update = check_and_cast<Update*>(msg);
    unsigned long senderId = update->getSenderId();
    int sequence = update->getSequence();
    std::string LCName = update->getLCName();

//    cout << fullName << " received update " << update << " from " << LCName << endl;

    boost::tuple<unsigned long, int> event_key(senderId, sequence);
    if (trace.count(event_key) == 0) {
        std::string content = update->getContent();
        trace[event_key] = content;

        // forward content to the application layer
        app->sendUpdateToClient(content);

        // for interaction latency calculation
        boost::tuple<unsigned long, std::string, int> update_key(senderId,
                LCName, sequence);
        if (sentEvents.count(update_key) > 0) {
            simtime_t sentTime = sentEvents[update_key];
            simtime_t latency = simTime() - sentTime;
            emit(interactionLatency, latency);
            // for event delivery rate calculation
            GlobalStatisticsAccess().get()->addReceivedEvent(fullName, LCName,
                    sequence);
        }
    }

    delete update;
}

void ClientCtrl::handleConfigUpdate(cMessage* msg) {

    ConfigUpdate* update = check_and_cast<ConfigUpdate*>(msg);
    std::string LCName = update->getLCName();
    std::string configList = update->getConfigs();
    set<std::string> replicas;
    util::convertStrToSet(configList, replicas);
    if (LCName.compare(this->LCName) == 0) {
        replicaMaps.erase(LCName);
        for (auto elem : replicas) {
            std::string replica = elem;
            IPvXAddress ipAddr(replica.c_str());
            util::multimap_insert(replicaMaps, { LCName, ipAddr });
        }
    } else {
        neighborMaps.erase(LCName);
        for (auto elem : replicas) {
            std::string replica = elem;
            IPvXAddress ipAddr(replica.c_str());
            util::multimap_insert(neighborMaps, { LCName, ipAddr });
        }
    }

    delete update;
}

void ClientCtrl::handleLoad() {

    // start checking connection
    flag = par("connect");
    if (!flag) {
        scheduleAt(simTime() + loadCycle, loadTimeout);
        EV << "I am a client~" << endl;
    } else {
        TransportAddress* rendAddr =
                UnderlayConfiguratorAccess().get()->createLogicComputer();
        rendezvous = rendAddr->getIp();
        Connect* connect = new Connect(msg::REQ_CONNECT);
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setSrcAddr(ipAddress);
        udpControlInfo->setDestAddr(rendezvous);
        connect->setControlInfo(udpControlInfo);
        // set logical computer name
        LCName = std::string(fullName) + "-LC";
        connect->setLCName(LCName.c_str());
        connect->setPeerId(app->getPeerId());
        sendIdeal(connect);
    }
}

void ClientCtrl::handleConnectReply(cMessage* msg) {
    ConnectReply* reply = check_and_cast<ConnectReply*>(msg);
    IPvXAddress srcAddr = check_and_cast<UDPControlInfo *>(
            reply->getControlInfo())->getSrcAddr();

    std::string LCName = reply->getLCName();
    std::string hostAddrs = reply->getHostAddrs();

    // to stop the attempt of connection once the connection has already been set up
    if (starts.count(LCName) == 0 || starts[LCName] < 0) {
        if (!hostAddrs.empty()) {
            vector<std::string> toAdd;
            util::splitString(hostAddrs, ";", toAdd);
            for (size_t i = 0; i < toAdd.size(); i++) {
                std::string addrStr = toAdd[i];
                IPvXAddress addr(addrStr.c_str());
                util::multimap_insert(replicaMaps, { LCName, addr });
            }
            rendezvousMaps.insert( { LCName, srcAddr });
            starts[LCName] = -1;
            seq[LCName] = 0;
        }

        ConnectTimeout* connectTimeout = new ConnectTimeout(
                msg::CONNECT_TIMEOUT);
        connectTimeout->setLCName(LCName.c_str());
        // in case of re-connect requested from nodes due to cycle un-started
        cancelEvent(connectTimeout);
        scheduleAt(simTime() + connectCycle, connectTimeout);
    }

    delete reply;
}

void ClientCtrl::handleHandShakeReject(cMessage* msg) {
    HandShakeReject* reply = check_and_cast<HandShakeReject*>(msg);
    IPvXAddress srcAddr = check_and_cast<UDPControlInfo *>(
            reply->getControlInfo())->getSrcAddr();

    std::string LCName = reply->getLCName();
    std::string hostAddrs = reply->getHostAddrs();

    // to stop the attempt of connection once the connection has already been set up
    if (starts.count(LCName) == 0 || starts[LCName] < 0) {
        if (!hostAddrs.empty()) {
            vector<std::string> toAdd;
            util::splitString(hostAddrs, ";", toAdd);
            for (size_t i = 0; i < toAdd.size(); i++) {
                std::string addrStr = toAdd[i];
                IPvXAddress addr(addrStr.c_str());
                util::multimap_insert(neighborMaps, { LCName, addr });
            }
            rendezvousMaps.insert( { LCName, srcAddr });
            starts[LCName] = -1;
            seq[LCName] = 0;
        }

        HandShakeTimeout* hsTimeout = new HandShakeTimeout(
                msg::HANDSHAKE_TIMEOUT);
        hsTimeout->setLCName(LCName.c_str());
        // in case of re-connect requested from nodes due to cycle un-started
        cancelEvent(hsTimeout);
        scheduleAt(simTime() + connectCycle, hsTimeout);
    }

    delete reply;
}

void ClientCtrl::handleConnectTimeout(cMessage* msg) {
    ConnectTimeout* connectTimeout = check_and_cast<ConnectTimeout*>(msg);
    std::string LCName = connectTimeout->getLCName();
    // init join
    pair<multimap<std::string, IPvXAddress>::iterator,
            multimap<std::string, IPvXAddress>::iterator> ret =
            replicaMaps.equal_range(LCName);
    for (multimap<std::string, IPvXAddress>::iterator iter = ret.first;
            iter != ret.second; iter++) {
        InitJoin* initJoin = new InitJoin(msg::INIT_JOIN);
        initJoin->setSenderId(id);
        initJoin->setSourceName(fullName);
        initJoin->setSeq(seq[LCName]);
        initJoin->setJoinTime(simTime());
        initJoin->setLCName(LCName.c_str());
        IPvXAddress ip = iter->second;
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(ip);
        udpControlInfo->setSrcAddr(ipAddress);
        initJoin->setControlInfo(udpControlInfo);
        sendIdeal(initJoin);
    }

    delete connectTimeout;
}

void ClientCtrl::handleHandShakeTimeout(cMessage* msg) {
    HandShakeTimeout* hsTimeout = check_and_cast<HandShakeTimeout*>(msg);
    std::string LCName = hsTimeout->getLCName();
    // init join
    pair<multimap<std::string, IPvXAddress>::iterator,
            multimap<std::string, IPvXAddress>::iterator> ret =
            neighborMaps.equal_range(LCName);

//    dout_cc << "in handleHandShakeTimeout" << endl;

    for (multimap<std::string, IPvXAddress>::iterator iter = ret.first;
            iter != ret.second; iter++) {
        HandShake* hs = new HandShake(msg::HANDSHAKE);
        hs->setSenderId(id);
        hs->setSourceName(fullName);
        hs->setSeq(seq[LCName]);
        hs->setJoinTime(simTime());
        hs->setLCName(LCName.c_str());
        IPvXAddress ip = iter->second;
        UDPControlInfo* udpControlInfo = new UDPControlInfo();
        udpControlInfo->setDestAddr(ip);
        udpControlInfo->setSrcAddr(ipAddress);
        hs->setControlInfo(udpControlInfo);
        sendIdeal(hs);

//        dout_cc << "send HandShakeTimeout" << endl;
    }

    delete hsTimeout;
}

set<IPvXAddress> ClientCtrl::getMeshReplicas() {
    set<IPvXAddress> replicas;
    pair<multimap<std::string, IPvXAddress>::iterator,
            multimap<std::string, IPvXAddress>::iterator> ret =
            replicaMaps.equal_range(LCName);
    for (multimap<std::string, IPvXAddress>::iterator iter = ret.first;
            iter != ret.second; iter++) {
        replicas.insert(iter->second);
    }
    return replicas;
}

std::string ClientCtrl::getLCName() {
    return LCName;
}

void ClientCtrl::finish() {
    appDataQueue.clear();
}
