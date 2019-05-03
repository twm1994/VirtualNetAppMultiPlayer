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

#include "NodeApp.h"
#include "../ctrl/NodeCtrl.h"

Define_Module(NodeApp);

NodeApp::NodeApp() :
        m_env(new MasterMap(), std::cout), position(3) {
    lastApplied = -1;
    initial_state = 0;
    STAGE_NUM = stage::NODE_APP_INIT;
    peer_id = 0;
    skin = 1;
    gear = 0;
    version = 0;
    reqId = 0;
    m_env.getMap().load();
    initialized = false;
    init_empty = false;
    init_start = false;
    terminating = false;

    persist = NULL;
}

NodeApp::~NodeApp() {
    if (persist != NULL) {
        cancelAndDelete(persist);
    }
}

int NodeApp::numInitStages() const {
    return STAGE_NUM;
}

void NodeApp::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();
        step_timer =
                getParentModule()->getSubmodule("ctrl")->par("roundCycle").doubleValue();

        WATCH(lastApplied);
        WATCH(initial_state);
        WATCH_MAP(Qa_key);
        WATCH_MAP(Qa);
        WATCH_MAP(Qs);
        WATCH(peer_id);
        WATCH_VECTOR(position);
        WATCH(skin);
        WATCH(gear);
        WATCH(version);
        WATCH(initialized);
    }
    if (stage == 1) {
        // create the host information for the P2P cloud service
        ipAddress = loadIPAddress(getParentModule()->par("IPInterface"),
                getParentModule());
        id = util::getSHA1(ipAddress.get4().str() + "4000",
                GlobalParametersAccess().get()->getAddrSpaceSize());
        SimpleNodeEntry* entry = new SimpleNodeEntry(this->getParentModule());
        HostInfo* info = new HostInfo(this->getParentModule()->getId(),
                this->getParentModule()->getFullName());
        info->setHostId(id);
        info->setEntry(entry);
        //add host to bootstrap oracle
        GlobalNodeListAccess().get()->addPeer(ipAddress, info);

        ctrl = dynamic_cast<NodeCtrl*>(this->getParentModule()->getSubmodule(
                "ctrl"));

        persist_cycle = par("pesist_cycle");
        persist = new cMessage(msg::APP_PERSIST);
    }
}

void NodeApp::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::EVENT_APPLY)) {
        EventApply* eventApply = check_and_cast<EventApply*>(msg);
        applyEvent(eventApply);
    } else if (msg->isName(msg::CHORD_REPLY)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        handleChordMsg(chordMsg);
    } else if (msg->isName(msg::APP_PERSIST)) {
        persistAppState();
    }
}

void NodeApp::initApp(unsigned short peerId) {
    this->peer_id = peerId;
}

void NodeApp::persistAppState() {
    // store state to the P2P cloud
    ChordInfo* chord = NULL;
    do {
        chord = GlobalNodeListAccess().get()->randChord();
    } while (chord == NULL
            || !GlobalNodeListAccess().get()->isReady(chord->getChordId()));

    ChordMessage* message = new ChordMessage(msg::CHORD_LOOK_UP);
    message->setSender(id);
    message->setHop(0);
    message->setType(ChordMsgType::CHORD_STORE);
    message->setKey(reqId);
    PlayerState ps;
    ps.peerId = this->peer_id;
    ps.position = this->position;
    ps.version = this->version;
    ps.gear = this->gear;
    ps.skin = this->skin;
    std::string serialized = ps.serialize();
    message->setContent(serialized.c_str());
    message->setByteLength(serialized.length());
    message->setLabel(msg::LABEL_CONTENT);
    IPvXAddress destAddr = GlobalNodeListAccess().get()->getNodeAddr(
            chord->getChordId());
    UDPControlInfo* udpControlInfo = new UDPControlInfo();
    udpControlInfo->setDestAddr(destAddr);
    message->setControlInfo(udpControlInfo);
    send(message, gateHalf("link", cGate::OUTPUT));

    scheduleAt(simTime() + persist_cycle, persist);
}

void NodeApp::handleChordMsg(ChordMessage* chordMsg) {
    int type = chordMsg->getType();
    if (type == ChordMsgType::CHORD_GET_REPLY) {
        std::string content = chordMsg->getContent();
        if (content.compare(data::DATA_EMPTY) != 0) {
            PlayerState ps;
            ps.deserialize(content);
            this->position = ps.position;
            this->skin = ps.skin;
            this->gear = ps.gear;
            this->version = ps.version;
        } else {
            std::cout << data::DATA_EMPTY << endl;
            init_empty = true;
        }

        initClientState();

    } else if (type == ChordMsgType::CHORD_STORE_REPLY) {
        if (terminating) {
            toExit = true;
            IPvXAddress rendezvous = loadIPAddress(
                    getParentModule()->par("RendezvousInterface"),
                    getParentModule());
            cMessage* msg = new cMessage(msg::TERMINATION);
            UDPControlInfo* udpControlInfo = new UDPControlInfo();
            udpControlInfo->setDestAddr(rendezvous);
            udpControlInfo->setSrcAddr(ipAddress);
            msg->setControlInfo(udpControlInfo);
            send(msg, gateHalf("link", cGate::OUTPUT));
        }
    }

    delete chordMsg;
}

void NodeApp::initClientState() {
    // look for the TOSERVER_INIT command
    boost::tuple<int, unsigned long, int> eventKey;
    unsigned long state = -1;
    for (auto elem : Qa) {
        std::string event = elem.second;
        if (event.find(event::TO_INIT)) {
            int index = elem.first;
            eventKey = Qa_key[index];
            state = Qs[index];
            break;
        }
    }

    // empty check
    if (get<1>(eventKey) != 0) {
        std::string response;
        if (!init_empty) {

//            std::cout << "Generating client init response" << endl;

            SharedBuffer<u8> reply(2 + 12 + 2 + 2);
            writeU16(&reply[0], TOCLIENT_INIT);
            v3s32 position2(position[0] * 100, position[1] * 100,
                    position[2] * 100);
            writeV3S32(&reply[2], position2);
            writeU16(&reply[2 + 12], skin);
            writeU16(&reply[2 + 12 + 2], gear);
            std::string result;
            util::binary_to_string(*reply, reply.getSize(), result);
            // go through channel 1
            response = result + "+" + std::to_string(reply.getSize()) + "+"
                    + "1" "+" + std::to_string(peer_id) + "+" + event::TO_INIT;
        } else {

//            std::cout << "Generating empty client init response" << endl;

            SharedBuffer<u8> reply(2);
            writeU16(&reply[0], TOCLIENT_INIT_EMPTY);
            std::string result;
            util::binary_to_string(*reply, reply.getSize(), result);
            // go through channel 1
            response = result + "+" + std::to_string(reply.getSize()) + "+"
                    + "1" "+" + std::to_string(peer_id) + "+" + event::TO_INIT;
        }

        UpdateApply* update = new UpdateApply(msg::UPDATE_APPLY);
        update->setEvent(response.c_str());
        update->setRound(get<0>(eventKey));
        update->setClientId(get<1>(eventKey));
        update->setSequence(get<2>(eventKey));
        update->setState(state);
        send(update, gateHalf("link", cGate::OUTPUT));

        initialized = true;

        // apply the rest events
        applyEvent();

        // start to cyclically persist application state
        scheduleAt(simTime() + persist_cycle, persist);
    }
}

// for event handling ============================================

void NodeApp::applyEvent(EventApply* eventApply) {
    // simulation step-wise advancement
    // step interval, in seconds
    double dtime = step_timer.dbl();
    m_env.step(dtime);

    // handle events
    if (eventApply) {
        // deliver event to the application layer
        std::string content = eventApply->getEvent();

//        cout << "app data content: " << content << endl;

        int index = -1;
        if (!Qa.empty()) {
            index = Qa.rbegin()->first;
        }
        index++;
        Qa[index] = content;
        boost::tuple<int, unsigned long, int> key(eventApply->getRound(),
                eventApply->getClientId(), eventApply->getSequence());
        Qa_key[index] = key;
    }

    // handle events at application layer
    std::map<int, std::string>::iterator it2 = Qa.begin();
    if (lastApplied != -1) {
        it2 = Qa.find(lastApplied);
        it2++;
    }
    for (; it2 != Qa.end(); it2++) {
        int index2 = it2->first;
        std::string event = it2->second;
        // generate response message
        // initialization check
        if (!initialized) {
            // label the TOSERVER_INIT command
            if (isInitCommand(event)
                    && event.find(event::TO_INIT) >= event.length()) {
                event = event + "+" + event::TO_INIT;
                Qa[index2] = event;
                if (!init_start) {
                    init_start = true;

//                    std::cout
//                            << "client not initialized, retrieve data from P2P cloud"
//                            << endl;

                    // query data from P2P cloud
                    ChordInfo* chord = NULL;
                    do {
                        chord = GlobalNodeListAccess().get()->randChord();
                    } while (chord == NULL
                            || !GlobalNodeListAccess().get()->isReady(
                                    chord->getChordId()));
                    ChordMessage* message = new ChordMessage(
                            msg::CHORD_LOOK_UP);
                    message->setSender(id);
                    message->setHop(1);
                    message->setType(ChordMsgType::CHORD_GET);
                    message->setLabel(msg::LABEL_CONTENT);
                    message->setKey(reqId);
                    message->setContent("");
                    UDPControlInfo* udpControlInfo = new UDPControlInfo();
                    udpControlInfo->setDestAddr(
                            GlobalNodeListAccess().get()->getNodeAddr(
                                    chord->getChordId()));
                    udpControlInfo->setSrcAddr(ipAddress);
                    message->setControlInfo(udpControlInfo);
                    send(message, gateHalf("link", cGate::OUTPUT));
                }
            }
            continue;

        } else {
            int index2 = it2->first;
            lastApplied = index2;
            std::string event = it2->second;
            boost::tuple<int, unsigned long, int> eventKey = Qa_key[index2];

            std::string newState;
            if (Qs.empty()) {
                unsigned long input = sdbm::encode(event.c_str());
                newState = util::longToStr(initial_state)
                        + util::longToStr(input);
            } else {
                std::map<int, unsigned long>::reverse_iterator last =
                        Qs.rbegin();
                unsigned long lastState = last->second;
                EV << "[Debug] lastState = " << lastState << endl;
                unsigned long input = sdbm::encode(event.c_str());
                EV << "[Debug] input = " << input << endl;
                newState = util::longToStr(lastState) + util::longToStr(input);
                EV << "[Debug] newState = " << newState << endl;
            }
            Qs[index2] = sdbm::encode(newState.c_str());
            unsigned long state = Qs[index2];

            if (event.find(event::TO_INIT) < event.length()) {

//                std::cout << "bypass TO_INIT event after initialization"
//                        << endl;

                continue;
            }

            std::string response = "";
            if (event.find(event::TO_REMOVE) == 0) {
                std::vector<std::string> container;
                util::splitString(event, "+", container);
                unsigned short peerId = util::strToUShort(container[1]);
                std::string neighbor = container[2];
                std::string clientId = container[3];
                std::string result = removeNeighbor(peerId, clientId);
                if (!result.empty()) {
                    response = result + "+" + event::TO_REMOVE + "+" + neighbor;
                }
            } else {
                std::vector<std::string> container;
                util::splitString(event, "+", container);
                std::string encoded = container[0];
                int datasize = util::strToInt(container[1]);
                unsigned char decoded[datasize];
                util::string_to_binary(encoded, decoded, datasize);
                if (container.size() == 5) {
                    unsigned short peerId = util::strToUShort(container[2]);
                    std::string neighbor = container[3];
                    std::string clientId = container[4];
                    std::string result = processData(decoded, datasize, peerId,
                            clientId);
                    if (!result.empty()) {
                        response = result + "+" + event::TO_JOIN + "+"
                                + neighbor;

//                        std::cout << fullName << " response result: "
//                                << response << endl;
                    }
                } else if (container.size() == 4) {
//                peer_id = util::strToUShort(container[2]);
                    std::string clientId = container[3];
                    response = processData(decoded, datasize, peer_id,
                            clientId);
                } else {
                    response = processData(decoded, datasize);
                }
            }

            // assign update to the queues in Qsend
            if (!response.empty()) {  // in case of TOSERVER_PLAYERPOS command
                UpdateApply* update = new UpdateApply(msg::UPDATE_APPLY);
                update->setEvent(response.c_str());
                update->setRound(get<0>(eventKey));
                update->setClientId(get<1>(eventKey));
                update->setSequence(get<2>(eventKey));
                update->setState(state);
                send(update, gateHalf("link", cGate::OUTPUT));
            }
        } // end of the initialized condition check
    } // end of the event apply loop

    delete eventApply;
}

bool NodeApp::isInitCommand(std::string event) {
    if (event.find(event::TO_REMOVE) != 0) {
        std::vector<std::string> container;
        util::splitString(event, "+", container);
        std::string encoded = container[0];
        int datasize = util::strToInt(container[1]);
        unsigned char decoded[datasize];
        util::string_to_binary(encoded, decoded, datasize);
        ToServerCommand command = (ToServerCommand) readU16(&decoded[0]);
        if (command == TOSERVER_INIT) {
            return true;
        }
    }

    return false;
}

std::string NodeApp::removeNeighbor(u16 peerId, std::string clientId) {

//    std::cout << "Server::removeNeighbor(): RMPLAYER" << std::endl;

    SharedBuffer<u8> reply(2 + 2);
    writeU16(&reply[0], TOCLIENT_RMPLAYER);
    writeU16(&reply[2], peerId);
// Send as unreliable
//            m_con.Send(peerId, 1, reply, true);
    std::string response;
    util::binary_to_string(*reply, reply.getSize(), response);
// go through channel 0
    std::string encoded = response + "+" + std::to_string(reply.getSize()) + "+"
            + "0" "+" + clientId;
    return encoded;
}

std::string NodeApp::processData(u8 *data, u32 datasize, u16 peerId,
        std::string clientId) {
// Let free access to the environment and the connection
//    JMutexAutoLock envlock(m_env_mutex);
//    JMutexAutoLock conlock(m_con_mutex);

    try {

        if (datasize < 2)
            return "";

        ToServerCommand command = (ToServerCommand) readU16(&data[0]);

        if (command == TOSERVER_GETBLOCK) {

            // Check for too short data
            if (datasize < 8)
                return "";
//            std::cout << "Server::ProcessData(): GETBLOCK" << std::endl;
            /*
             Get block data and send it
             */
            v3s16 p;
            p.X = readS16(&data[2]);
            p.Y = readS16(&data[4]);
            p.Z = readS16(&data[6]);
            MapBlock *block = m_env.getMap().getBlock(p);

            u32 replysize = 8 + MapBlock::serializedLength();
            SharedBuffer<u8> reply(replysize);
            writeU16(&reply[0], TOCLIENT_BLOCKDATA);
            writeS16(&reply[2], p.X);
            writeS16(&reply[4], p.Y);
            writeS16(&reply[6], p.Z);
            block->serialize(&reply[8]);
            // Send as unreliable
//            m_con.Send(peerId, 1, reply, true);
            std::string response;
            util::binary_to_string(*reply, reply.getSize(), response);
            // go through channel 1
            std::string encoded = response + "+"
                    + std::to_string(reply.getSize()) + "+" + "1" + "+"
                    + event::GET_BLOCK + "+" + clientId;
            return encoded;
        } else if (command == TOSERVER_REMOVENODE) {

//            std::cout << "Server::ProcessData(): ADDNODE" << std::endl;

            if (datasize < 8)
                return "";

            v3s16 p;
            p.X = readS16(&data[2]);
            p.Y = readS16(&data[4]);
            p.Z = readS16(&data[6]);

            MapNode n;
            n.d = MATERIAL_AIR;
            v3s16 blockPos = m_env.getMap().getNodeBlockPos(p);
            m_env.getMap().getBlock(blockPos);
            m_env.getMap().setNode(p, n);

            u32 replysize = 8;
            SharedBuffer<u8> reply(replysize);
            writeU16(&reply[0], TOCLIENT_REMOVENODE);
            writeS16(&reply[2], p.X);
            writeS16(&reply[4], p.Y);
            writeS16(&reply[6], p.Z);
            // Send as reliable
//            m_con.SendToAll(0, reply, true);
            std::string response;
            util::binary_to_string(*reply, reply.getSize(), response);
            // go through channel 0
            std::string encoded = response + "+"
                    + std::to_string(reply.getSize()) + "+" + "0";
            return encoded;
        } else if (command == TOSERVER_ADDNODE) {

//            std::cout << "Server::ProcessData(): ADDNODE" << std::endl;

            if (datasize < 8 + MapNode::serializedLength())
                return "";

            v3s16 p;
            p.X = readS16(&data[2]);
            p.Y = readS16(&data[4]);
            p.Z = readS16(&data[6]);

            MapNode n;
            n.deSerialize(&data[8]);
            v3s16 blockPos = m_env.getMap().getNodeBlockPos(p);
            m_env.getMap().getBlock(blockPos);
            m_env.getMap().setNode(p, n);

            u32 replysize = 8 + MapNode::serializedLength();
            SharedBuffer<u8> reply(replysize);
            writeU16(&reply[0], TOCLIENT_ADDNODE);
            writeS16(&reply[2], p.X);
            writeS16(&reply[4], p.Y);
            writeS16(&reply[6], p.Z);
            n.serialize(&reply[8]);
            // Send as reliable
//            m_con.SendToAll(0, reply, true);
            std::string response;
            util::binary_to_string(*reply, reply.getSize(), response);
            // go through channel 0
            std::string encoded = response + "+"
                    + std::to_string(reply.getSize()) + "+" + "0";
            return encoded;
        } else if (command == TOSERVER_PLAYERPOS) {
            if (datasize < 2 + 12 + 12 + 12 + 2 + 2)
                return "";

//            cout << "Server::ProcessData(): update player position" << endl;

            Player *player = m_env.getPlayer(peerId);

            // Create a player if it doesn't exist
            if (player == NULL) {
//                std::cout << "Server::ProcessData(): Adding player " << peerId
//                        << std::endl;
                player = new Player(peerId == this->peer_id ? true : false);
                player->peer_id = peerId;
                m_env.addPlayer(player);
            }

            // TODO: player couter is not needed
            player->timeout_counter = 0.0;

            // update position and speed
            v3s32 ps = readV3S32(&data[2]);

//            cout << "player position: " << ps.X << "-" << ps.Y << "-" << ps.Z
//                    << endl;

            v3s16 pos = v3s16((s16) (ps.X / (100 * MAP_BLOCKSIZE)),
                    (s16) (ps.Y / (100 * MAP_BLOCKSIZE)),
                    (s16) (ps.Z / (100 * MAP_BLOCKSIZE)));

//            cout << "block position: " << pos.X << "-" << pos.Y << "-" << pos.Z
//                    << endl;

            m_env.getMap().getBlock(pos);

            v3s32 ss = readV3S32(&data[2 + 12]);
            v3s32 rs = readV3S32(&data[2 + 12 + 12]);
            v3f position((f32) ps.X / 100., (f32) ps.Y / 100.,
                    (f32) ps.Z / 100.);
            v3f speed((f32) ss.X / 100., (f32) ss.Y / 100., (f32) ss.Z / 100.);
            v3f rotation((f32) rs.X / 100., (f32) rs.Y / 100.,
                    (f32) rs.Z / 100.);
            player->setPosition(position);
            player->speed = speed;
            player->setRotation(rotation);
            player->skin = readU16(&data[2 + 12 + 12 + 12]);
            player->gear = readU16(&data[2 + 12 + 12 + 12 + 2]);

            // update state only if there is any state change
            if (player->isLocal()) {
                float x = position.X;
                float y = position.Y;
                float z = position.Z;
                unsigned short s = player->skin;
                unsigned short g = player->gear;
                if (this->position[0] != x || this->position[1] != y
                        || this->position[2] != z || skin != s || gear != g) {
                    version++;

                    this->position[0] = x;
                    this->position[1] = y;
                    this->position[2] = z;
                    skin = s;
                    gear = g;
                }
            }

            // generate the player position update command
//            cout << "player position:" << position.X << ", " << position.Y
//                    << ", " << position.Z << endl;
//            cout << "player skin:" << skin << endl;
//            cout << "player gear:" << gear << endl;

            u32 datasize = 2 + 2 + 12 + 12 + 12 + 2 + 2;
            SharedBuffer<u8> reply(datasize);
            writeU16(&reply[0], TOCLIENT_PLAYERPOS);
            u32 start = 2;
            v3f pf = player->getPosition();
            v3s32 position2(pf.X * 100, pf.Y * 100, pf.Z * 100);
            // TODO: here, speed may be set to 0
            v3f sf = player->speed;
            v3s32 speed2(sf.X * 100, sf.Y * 100, sf.Z * 100);
            v3f rf = player->getRotation();
            v3s32 rotation2(rf.X * 100, rf.Y * 100, rf.Z * 100);
            writeU16(&reply[start], player->peer_id);
            writeV3S32(&reply[start + 2], position2);
            writeV3S32(&reply[start + 2 + 12], speed2);
            writeV3S32(&reply[start + 2 + 12 + 12], rotation2);
            // send player state
            writeU16(&reply[start + 2 + 12 + 12 + 12], player->skin);
            writeU16(&reply[start + 2 + 12 + 12 + 12 + 2], player->gear);
            // Send as reliable
            //            m_con.SendToAll(0, data, false)
            std::string response;
            util::binary_to_string(*reply, reply.getSize(), response);
            // go through channel 0
            std::string encoded = response + "+"
                    + std::to_string(reply.getSize()) + "+" + "0" + "+"
                    + clientId;

//            cout << "command for adding new player: " << encoded << endl;

            return encoded;

        } else if (command == TOSERVER_EXIT) {

//            std::cout << "Server::ProcessData(): CLIENT_EXIT" << std::endl;

            onExit();
        } else {
            std::cout << "WARNING: Server::ProcessData(): Ingoring "
                    "unknown command " << command << std::endl;
        }

    } //try
    catch (SendFailedException &e) {
        std::cout << "Server::ProcessData(): SendFailedException: " << "what="
                << e.what() << std::endl;
    }
    return "";
}

void NodeApp::onExit() {

    Enter_Method_Silent("On node exit");

    terminating = true;

// store state to the P2P cloud
    ChordInfo* chord = NULL;
    do {
        chord = GlobalNodeListAccess().get()->randChord();
    }while (chord == NULL
    || !GlobalNodeListAccess().get()->isReady(chord->getChordId()));

    ChordMessage* message = new ChordMessage(msg::CHORD_LOOK_UP);
    message->setSender(id);
    message->setHop(0);
    message->setType(ChordMsgType::CHORD_STORE);
    message->setKey(reqId);
    PlayerState ps;
    ps.peerId = this->peer_id;
    ps.position = this->position;
    ps.version = this->version;
    ps.gear = this->gear;
    ps.skin = this->skin;
    std::string serialized = ps.serialize();
    message->setContent(serialized.c_str());
    message->setByteLength(serialized.length());
    message->setLabel(msg::LABEL_CONTENT);
    IPvXAddress destAddr = GlobalNodeListAccess().get()->getNodeAddr(
    chord->getChordId());
    UDPControlInfo* udpControlInfo = new UDPControlInfo();
    udpControlInfo->setDestAddr(destAddr);
    message->setControlInfo(udpControlInfo);
    send(message, gateHalf("link", cGate::OUTPUT));
}

// for state recovery ============================================

std::string NodeApp::packState() {
    PlayerState ps;
    ps.peerId = this->peer_id;
    ps.gear = this->gear;
    ps.position = this->position;
    ps.skin = this->skin;
    ps.version = this->version;

    std::string state = ps.serialize();

    return state;
}

std::string NodeApp::packLastQState() {
    std::string lastQState;
    if (lastApplied > -1) {
        stringstream ss;
        boost::tuple<int, unsigned long, int> key = Qa_key[lastApplied];
        ss << lastApplied << "|" << get<0>(key) << "-" << get<1>(key) << "-"
                << get<2>(key) << "|" << Qa[lastApplied] << "|"
                << Qs[lastApplied];
        lastQState = ss.str();
    }
    return lastQState;
}

void NodeApp::unpackState(string state) {
    PlayerState ps;
    ps.deserialize(state);
    if (ps.peerId != 0) {
        this->peer_id = ps.peerId;
        this->position = ps.position;
        this->skin = ps.skin;
        this->gear = ps.gear;
        this->version = ps.version;
        Player* player = new Player(true);
        player->peer_id = this->peer_id;
        v3f p(this->position[0], this->position[1], this->position[2]);
        player->setPosition(p);
        this->m_env.addPlayer(player);

        this->reqId = util::getSHA1(std::to_string(peer_id),
                GlobalParametersAccess().get()->getAddrSpaceSize());

        initialized = true;
        init_start = true;
    }
}

void NodeApp::unpackLastQState(std::string lastQState) {
    if (!lastQState.empty()) {
        vector<string> container;
        util::splitString(lastQState, "|", container);
        if (container.size() == 4) {
            this->lastApplied = util::strToUShort(container[0]);
            vector<string> subcontainer;
            util::splitString(container[1], "-", subcontainer);
            int round = util::strToInt(subcontainer[0]);
            unsigned long senderId = util::strToLong(subcontainer[1]);
            int seq = util::strToInt(subcontainer[2]);
            boost::tuple<int, unsigned long, int> key(round, senderId, seq);
            Qa_key[lastApplied] = key;
            Qa[lastApplied] = container[2];
            Qs[lastApplied] = util::strToLong(container[3]);
        }
    }
}

void NodeApp::initState(unsigned long state) {
    this->initial_state = state;
    Qs.clear();
    Qa.clear();
    Qa_key.clear();
    lastApplied = -1;
}
