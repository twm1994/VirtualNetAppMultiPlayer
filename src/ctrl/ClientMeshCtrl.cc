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

#include "ClientMeshCtrl.h"

s16 g_viewing_range_nodes = MAP_BLOCKSIZE;

Define_Module(ClientMeshCtrl);

ClientMeshCtrl::ClientMeshCtrl() :
        m_env(new MasterMap(), std::cout) {
    STAGE_NUM = 2;
    // NPC simulation step interval
    cycle = 1.0;
    skin = 99;
    gear = 0;
    updatePosition = NULL;
}

ClientMeshCtrl::~ClientMeshCtrl() {
    if (updatePosition != NULL) {
        cancelAndDelete(updatePosition);
    }
}

int ClientMeshCtrl::numInitStages() const {
    return STAGE_NUM;
}

void ClientMeshCtrl::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();
        LCName = fullName;
        peerId = pearsonHash16(fullName);
        // initialize node ID
//        id = sdbm::encode(fullName);
        id = (unsigned long) peerId;

        // retrieve parameters and initialize timers
        connectCycle = par("initJoinTime").doubleValue();

        WATCH(id);
        WATCH(peerId);
        WATCH_SET(neighborsToAdd);
        WATCH_SET(neighborsToRemove);
        WATCH_MULTIMAP(replicaMaps);
        WATCH_MULTIMAP(neighborMaps);
        WATCH_SET(neighbors);
        WATCH(LCName);
    }

    if (stage == 1) {
        TransportAddress* addr =
                UnderlayConfiguratorAccess().get()->registerEndpoint(
                        getParentModule());
        ipAddress = addr->getIp();

        cout << "ipAddress: " << ipAddress << endl;

        // initialize the neighbor location
        CoordinatorAccess().get()->setRendezvous(fullName, ipAddress);

        // initialize dumb player with NPC
        NPC *npc = new NPC();
        ClientInfo info = CoordinatorAccess().get()->getClientInfo(fullName);
        Coordinate c(info.getX(), info.getY());
        Coordinate location = CoordinatorAccess().get()->mapLocationBack(c);
        v3f position((f32) location.x, (f32) 2 * BS, (f32) location.y);

        npc->setPosition(position);
        m_env.setNPC(npc);
        // initialize the map blocks
        initMap(position);

        // initialize vertical position for NPC landing
        f32 oldY;
        do {
            oldY = npc->getPosition().Y;
            m_env.step(cycle.dbl());
        } while (oldY != npc->getPosition().Y);

        cout << "NPC position: " << npc->getPosition().X << ", "
                << npc->getPosition().Y << ", " << npc->getPosition().Z << endl;

        updatePosition = new cMessage(msg::NPC_UPDATE_POS);
        scheduleAt(simTime() + cycle, updatePosition);
    }
}

void ClientMeshCtrl::initMap(v3f position) {

    core::aabbox3d<s16> box_nodes(Map::floatToInt(position));

    s16 d = g_viewing_range_nodes;
    box_nodes.MinEdge -= v3s16(d, d, d);
    box_nodes.MaxEdge += v3s16(d, d, d);

    core::aabbox3d<s16> box_blocks = core::aabbox3d<s16>(
            m_env.getMap().getNodeBlockPos(box_nodes.MinEdge),
            m_env.getMap().getNodeBlockPos(box_nodes.MaxEdge));
    core::list<MapBlock *> blocks_changed;
    core::list<MapBlock *>::Iterator bi;

    for (s16 y = box_blocks.MaxEdge.Y; y >= box_blocks.MinEdge.Y; y--) {
        for (s16 z = box_blocks.MinEdge.Z; z <= box_blocks.MaxEdge.Z; z++) {
            for (s16 x = box_blocks.MinEdge.X; x <= box_blocks.MaxEdge.X; x++) {
                // This intentionally creates new blocks on demand
                try {
                    MapBlock * block = m_env.getMap().getBlock(v3s16(x, y, z));
                } catch (InvalidPositionException &e) {
                }
            }
        }
    }
}

void ClientMeshCtrl::finish() {
    disposeTimeouts();
}

void ClientMeshCtrl::disposeTimeouts() {
    try {
        for (map<string, ClientCycleTimeout*>::iterator it = timeouts.begin();
                it != timeouts.end(); ++it) {
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

void ClientMeshCtrl::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::NEIGHBOR_ADD)) {
        handleAddNeighbor(msg);
    } else if (msg->isName(msg::NEIGHBOR_RM)) {
        handleRMNeighbor(msg);
    } else if (msg->isName(msg::HANDSHAKE_TIMEOUT)) {
        handleHandShakeTimeout(msg);
    } else if (msg->isName(msg::JOIN)) {
        handleJoin(msg);
    } else if (msg->isName(msg::EVENT_CYCLE)) {
        handleCycleEvent(msg);
    } else if (msg->isName(msg::UPDATE)) {
        // currently do nothing
        delete msg;
    } else if (msg->isName(msg::HANDSHAKE_REJECT)) {
        handleHandShakeReject(msg);
    } else if (msg->isName(msg::CONFIG_UPDATE)) {
        handleConfigUpdate(msg);
    } else if (msg->isName(msg::NPC_UPDATE_POS)) {
        updatePlayerPosition(msg);
    }
}

void ClientMeshCtrl::handleAddNeighbor(cMessage* msg) {
    // for neighbor join during the Mesh init phase
    if (this->LCName.empty()) {
        delete msg;
        return;
    }

    AddNeighbor* addNeighbor = check_and_cast<AddNeighbor*>(msg);

    string LCName = addNeighbor->getNeighbor();
    string hostAddrs = addNeighbor->getHostAddrs();

    // to stop the attempt of connection once the connection has already been set up
    if (neighborsToAdd.count(LCName) == 0) {
        if (!hostAddrs.empty()) {
            vector<string> toAdd;
            util::splitString(hostAddrs, ";", toAdd);
            for (size_t i = 0; i < toAdd.size(); i++) {
                string addrStr = toAdd[i];
                IPvXAddress addr(addrStr.c_str());
                util::multimap_insert(neighborMaps, { LCName, addr });
            }
            starts[LCName] = -1;
            seq[LCName] = 0;
            neighborsToAdd.insert(LCName);
            neighborsToRemove.erase(LCName);
            neighbors.insert(LCName);
        }

        HandShakeTimeout* hsTimeout = new HandShakeTimeout(
                msg::HANDSHAKE_TIMEOUT);
        hsTimeout->setLCName(LCName.c_str());
        // in case of re-connect requested from nodes due to cycle un-started
        cancelEvent(hsTimeout);
        scheduleAt(simTime() + connectCycle, hsTimeout);

        cout << fullName << " Time to connect to neighbor" << endl;
    } else {
        cout << fullName << " Connecting to neighbor has been timed" << endl;
    }

    delete addNeighbor;
}

void ClientMeshCtrl::handleRMNeighbor(cMessage* msg) {
    // for neighbor leave during the Mesh init phase
    if (this->LCName.empty()) {
        delete msg;
        return;
    }

    RemoveNeighbor* rmNeighbor = check_and_cast<RemoveNeighbor*>(msg);

    string LCName = rmNeighbor->getNeighbor();
    string hostAddrs = rmNeighbor->getHostAddrs();

    if (neighborsToRemove.count(LCName) == 0) {
        cout << fullName << " To remove neighbor: " << LCName << endl;

        neighborsToRemove.insert(LCName);
        starts.erase(LCName);
    }

    delete rmNeighbor;
}

void ClientMeshCtrl::handleHandShakeReject(cMessage* msg) {
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

void ClientMeshCtrl::handleHandShakeTimeout(cMessage* msg) {
    HandShakeTimeout* hsTimeout = check_and_cast<HandShakeTimeout*>(msg);
    string LCName = hsTimeout->getLCName();
    // init join
    pair<multimap<string, IPvXAddress>::iterator,
            multimap<string, IPvXAddress>::iterator> ret =
            neighborMaps.equal_range(LCName);
    for (multimap<string, IPvXAddress>::iterator iter = ret.first;
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

        cout << fullName << " connect to a Mesh replica: " << ip.str() << endl;
    }

    delete hsTimeout;
}

void ClientMeshCtrl::handleJoin(cMessage* msg) {
    Join* join = check_and_cast<Join*>(msg);
    string LCName = join->getLCName();
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

void ClientMeshCtrl::handleCycleEvent(cMessage* msg) {
    ClientCycleTimeout* cycleTimeout = check_and_cast<ClientCycleTimeout*>(msg);
    string LCName = cycleTimeout->getLCName();

    Event* em;
    em = new Event(msg::EVENT);
    em->setSenderId(id);
    stringstream ss;

    string event = getPlayerPosition();
    if (neighborsToRemove.count(LCName) > 0) {
        event = string(event::TO_REMOVE) + "+" + to_string(peerId) + "+"
                + fullName + "+" + to_string(id);

        cout << "Remove client event: " << event << endl;
    }

    em->setContent(event.c_str());
    // sourceName and destName must be set for module purge
    em->setSourceName(fullName);
    em->setSeq(seq[LCName]);
    // message size: event size + 1 byte for event sequence number
    em->setByteLength(event.size() + 1);

    // broadcast the message to all live nodes
    set<IPvXAddress> replicas;
    pair<multimap<string, IPvXAddress>::iterator,
            multimap<string, IPvXAddress>::iterator> ret =
            neighborMaps.equal_range(LCName);
    for (multimap<string, IPvXAddress>::iterator iter = ret.first;
            iter != ret.second; iter++) {
        replicas.insert(iter->second);
    }
    multicast(em, replicas);

    if (neighborsToRemove.count(LCName) > 0) {
        replicaMaps.erase(LCName);
        neighborsToRemove.erase(LCName);
        neighborsToAdd.erase(LCName);
        seq.erase(LCName);
        delete cycleTimeout;
        timeouts.erase(LCName);
        neighbors.erase(LCName);
    } else {
        // Set the timer for sending the next event
        scheduleAt(simTime() + cycle, cycleTimeout);
        // increase event sequence number
        seq[LCName]++;
    }
}

string ClientMeshCtrl::getPlayerPosition() {

    NPC* npc = m_env.getNPC();
    v3f pf = npc->getPosition();
    v3f rf = npc->getRotation();
    v3f sf = npc->speed;

//    cout << "NPC position: " << pf.X << ", " << pf.Y << ", " << pf.Z << endl;

    v3s32 position(pf.X * 100, pf.Y * 100, pf.Z * 100);
    v3s32 speed(sf.X * 100, sf.Y * 100, sf.Z * 100);
    v3s32 rotation(rf.X * 100, rf.Y * 100, rf.Z * 100);

    v3s16 pos = v3s16((s16) (position.X / (100 * MAP_BLOCKSIZE)),
            (s16) (position.Y / (100 * MAP_BLOCKSIZE)),
            (s16) (position.Z / (100 * MAP_BLOCKSIZE)));
    m_env.getMap().getBlock(pos);

    SharedBuffer<u8> data(2 + 12 + 12 + 12 + 2 + 2);
    writeU16(&data[0], TOSERVER_PLAYERPOS);
    writeV3S32(&data[2], position);
    writeV3S32(&data[2 + 12], speed);
    writeV3S32(&data[2 + 12 + 12], rotation);
    writeU16(&data[2 + 12 + 12 + 12], skin);
    writeU16(&data[2 + 12 + 12 + 12 + 2], gear);
    int datasize = data.getSize();

    string encoded;
    util::binary_to_string(*data, datasize, encoded);
    string event = encoded + "+" + to_string(datasize) + "+" + to_string(peerId)
            + "+" + fullName + "+" + to_string(id);

    return event;
}

void ClientMeshCtrl::updatePlayerPosition(cMessage* msg) {
    // update simulation first
    m_env.step(cycle.dbl());

    // update coordinator record and map position
    NPC* npc = m_env.getNPC();
    v3f pf = npc->getPosition();
    v3s32 position(pf.X * 100, pf.Y * 100, pf.Z * 100);
    Coordinate ps((long) position.X / 100, (long) position.Z / 100);
    Coordinate c = CoordinatorAccess().get()->mapLocation(ps);
    displayPosition(c.x, c.y);
    CoordinatorAccess().get()->updatePosition(fullName, c.x, c.y, ipAddress,
            LCName, neighbors, meshReplicas);

    scheduleAt(simTime() + cycle, updatePosition);
}

void ClientMeshCtrl::displayPosition(long x, long y) {
    getParentModule()->getDisplayString().setTagArg("p", 0, x);
    getParentModule()->getDisplayString().setTagArg("p", 1, y);
}

void ClientMeshCtrl::handleConfigUpdate(cMessage* msg) {

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

