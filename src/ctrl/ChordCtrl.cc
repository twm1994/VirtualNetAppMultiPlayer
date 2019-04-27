//
// Copyright (C) 2006 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include <fstream>
#include "ChordCtrl.h"

std::ofstream dfile_ch("VirtualNet_P2PCloudCtrl.txt");
std::ostream dout_ch(dfile_ch.rdbuf());

Define_Module(ChordCtrl);

ChordCtrl::ChordCtrl() {
    fingerToFix = 0;
//    maintainer = new cMessage(msg::CHORD_MAINT);
}

void ChordCtrl::final() {
    ;
}

ChordCtrl::~ChordCtrl() {
    // destroy self timer messages
//    cancelAndDelete(maintainer);
}

int ChordCtrl::numInitStages() const {
    return 2;
}

void ChordCtrl::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();
        M = GlobalParametersAccess().get()->getAddrSpaceSize();
        MAXid = pow(2, M);
        succSize = GlobalParametersAccess().get()->getSuccListSize();
        successorList.resize(succSize);
        fingerTable.resize(M);
        maintain_cycle = par("maintain_cycle");
        objectSize = par("object_size");

        WATCH(MAXid);
        WATCH(chordId);
        WATCH(predecessor);
        WATCH_VECTOR(successorList);
        WATCH_VECTOR(fingerTable);
        WATCH_MAP(p_state);
    } else if (stage == 1) {
        getParentModule()->getDisplayString().setTagArg("t", 0,
                to_string(chordId).c_str());
    }
}

bool ChordCtrl::operator<(const ChordCtrl& chord) const {
    return this->chordId < chord.chordId;
}

void ChordCtrl::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::CHORD_LOOK_UP)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onRoute(chordMsg);
    } else if (msg->isName(msg::CHORD_SUCCESSOR)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onRoute(chordMsg);
    } else if (msg->isName(msg::CHORD_SUCCESSOR_FOUND)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onSuccessorFound(chordMsg);
    } else if (msg->isName(msg::CHORD_FINAL)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onFinal(chordMsg);
    } else if (msg->isName(msg::CHORD_NOTIFY)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onNotify(chordMsg);
    } else if (msg->isName(msg::CHORD_QUERY_SUCCESSOR)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onQuerySuccessor(chordMsg);
    } else if (msg->isName(msg::CHORD_MAINT)) {
        maintain(msg);
    } else if (msg->isName(msg::CHORD_STORE)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onStore(chordMsg);
    } else if (msg->isName(msg::CHORD_GET)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onGet(chordMsg);
    } else if (msg->isName(msg::CHORD_REPLICATE)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onReplicate(chordMsg);
    } else if (msg->isName(msg::CHORD_FIX_REPLICA)) {
        ChordMessage* chordMsg = check_and_cast<ChordMessage*>(msg);
        onFixReplicas(chordMsg);
    }
}

void ChordCtrl::r_transmit(ChordMessage* msg, unsigned long destID) {
    msg->removeControlInfo();
    IPvXAddress srcAddr = GlobalNodeListAccess().get()->getNodeAddr(chordId);
    IPvXAddress destAddr = GlobalNodeListAccess().get()->getNodeAddr(destID);
    UDPControlInfo* udpControlInfo = new UDPControlInfo();
    udpControlInfo->setDestAddr(destAddr);
    udpControlInfo->setSrcAddr(srcAddr);
    msg->setControlInfo(udpControlInfo);
    string label = msg->getLabel();
    HostBase::r_transmit(msg, destAddr);
}

void ChordCtrl::onRoute(ChordMessage* msg) {
    string content = msg->getContent();
    unsigned long target = msg->getKey();

    if (msg->getType() == ChordMsgType::CHORD_GET
            && inAB(target, predecessor, chordId)) {
        ChordMessage* getmsg = new ChordMessage(msg::CHORD_GET);
        getmsg->setKey(target);
        getmsg->setContent(content.c_str());
        getmsg->setType(msg->getType());
        getmsg->setHop(msg->getHop());
        getmsg->setLabel(msg->getLabel());
        getmsg->setSender(msg->getSender());
        onGet(getmsg);
        delete msg;
    } else if (target == chordId
            || ((msg->getType() == ChordMsgType::SUCCESSOR
                    || msg->getType() == ChordMsgType::CHORD_STORE
                    || msg->getType() == ChordMsgType::CHORD_GET)
                    && inAB(target, chordId, successorList[0]))) {
        if (msg->getType() == ChordMsgType::SUCCESSOR && target == chordId) {
            ChordMessage* finalmsg = new ChordMessage(
                    msg::CHORD_SUCCESSOR_FOUND);
            finalmsg->setKey(target);
            finalmsg->setType(ChordMsgType::SUCCESSOR_FOUND);
            FastWriter fw;
            string s_profile = fw.write(profile());
            finalmsg->setContent(s_profile.c_str());
            finalmsg->setHop(msg->getHop() + 1);
            finalmsg->setLabel(msg->getLabel());
            finalmsg->setSender(chordId);
            r_transmit(finalmsg, msg->getSender());
        } else if (msg->getType() == ChordMsgType::SUCCESSOR
                && inAB(target, chordId, successorList[0])) {
            ChordMessage* querymsg = new ChordMessage(
                    msg::CHORD_QUERY_SUCCESSOR);
            querymsg->setKey(target);
            querymsg->setContent(content.c_str());
            querymsg->setType(ChordMsgType::QUERY_SUCCESSOR);
            querymsg->setHop(msg->getHop() + 1);
            querymsg->setLabel(msg->getLabel());
            querymsg->setSender(msg->getSender());
            r_transmit(querymsg, successorList[0]);
        } else { // for look_up messages
            if (msg->getType() == ChordMsgType::CHORD_STORE) {
                ChordMessage* storemsg = new ChordMessage(msg::CHORD_STORE);
                storemsg->setKey(target);
                storemsg->setContent(content.c_str());
                storemsg->setType(msg->getType());
                storemsg->setHop(msg->getHop() + 1);
                storemsg->setLabel(msg->getLabel());
                storemsg->setSender(msg->getSender());
                r_transmit(storemsg, successorList[0]);
            } else if (msg->getType() == ChordMsgType::CHORD_GET) {
                ChordMessage* getmsg = new ChordMessage(msg::CHORD_GET);
                getmsg->setKey(target);
                getmsg->setContent(content.c_str());
                getmsg->setType(msg->getType());
                getmsg->setHop(msg->getHop() + 1);
                getmsg->setLabel(msg->getLabel());
                getmsg->setSender(msg->getSender());
                r_transmit(getmsg, successorList[0]);
            } else {
                ChordMessage* finalmsg = new ChordMessage(msg::CHORD_FINAL);
                finalmsg->setKey(target);
                finalmsg->setType(ChordMsgType::CHORD_FINAL);
                // TODO content for look_up message final
                //            finalmsg->setContent(profile.dump().c_str());
                finalmsg->setHop(msg->getHop() + 1);
                finalmsg->setLabel(msg->getLabel());
                finalmsg->setSender(chordId);
                r_transmit(finalmsg, msg->getSender());
            }
        }
        delete msg;
    } else {
        unsigned long dest = closestPrecedingNode(target);
        if (dest == MAXid + 1) {

            cout << simTime() << " lookup " << target << " failed on "
                    << chordId << " with label '" << msg->getLabel()
                    << "' at hop " << msg->getHop() << ", due to max hop "
                    << (MAXid + 1) << " reached for " << chordId << endl;

            int replyType = ChordMsgType::CHORD_LOOK_UP;
            if (replyType == ChordMsgType::CHORD_STORE) {
                replyType = ChordMsgType::CHORD_STORE_REPLY;
            } else if (replyType == ChordMsgType::CHORD_GET) {
                replyType = ChordMsgType::CHORD_GET_REPLY;
            }
            handleFailure(msg, replyType);
            delete msg;
        } else {
            if (msg->getType() == ChordMsgType::CHORD_LOOK_UP
                    || msg->getType() == ChordMsgType::CHORD_STORE
                    || msg->getType() == ChordMsgType::CHORD_GET) {
                msg->setHop(msg->getHop() + 1);
            }
            r_transmit(msg, dest);
        }
    }
}

void ChordCtrl::onQuerySuccessor(ChordMessage* msg) {
    ChordMessage* finalmsg = new ChordMessage(msg::CHORD_SUCCESSOR_FOUND);
    finalmsg->setType(ChordMsgType::SUCCESSOR_FOUND);
    FastWriter fw;
    string s_profile = fw.write(profile());
    finalmsg->setContent(s_profile.c_str());
    finalmsg->setHop(msg->getHop() + 1);
    finalmsg->setLabel(msg->getLabel());
    finalmsg->setSender(chordId);
    r_transmit(finalmsg, msg->getSender());
    delete msg;
}

void ChordCtrl::onFinal(ChordMessage* msg) {
    delete msg;
}

void ChordCtrl::onSuccessorFound(ChordMessage* msg) {
    string label = msg->getLabel();
    string content = msg->getContent();
    Reader reader;
    Value succ;
    reader.parse(content, succ);
    bool updatePredecessor = false;
    if (label.find("successor") != string::npos) {  //predecessor
        unsigned long pred = succ["predecessor"].asUInt();
        if (label.find("first") != string::npos || pred == chordId
//                || !GlobalNodeListAccess().get()->isUp(pred)
                ) { // for node join or successor lookup
            successorList.resize(succSize);
            successorList[0] = succ["chordId"].asUInt();
            if (label.find("first") != string::npos) {
                predecessor = pred;
                GlobalNodeListAccess().get()->ready(chordId);
                startMaint();
            }

            vector<unsigned long> receivedSuccList;
            for (Value::const_iterator it = succ["successorList"].begin();
                    it != succ["successorList"].end(); it++) {
                receivedSuccList.push_back(it->asUInt());
            }

            std::copy(receivedSuccList.begin(), receivedSuccList.end() - 1,
                    successorList.begin() + 1);
            updatePredecessor = true;

            // in case the predecessor of the first successor failed
            if (pred != chordId) {
                notify(successorList[0]);
            }
        } else if (label.find("stabilize") != string::npos) {
            if (inAB(pred, chordId, succ["chordId"].asUInt())) { // in case a new node is found
                successorList.resize(succSize);
                successorList[0] = pred;
                successorList[1] = succ["chordId"].asUInt();

                vector<unsigned long> receivedSuccList;
                for (Value::const_iterator it = succ["successorList"].begin();
                        it != succ["successorList"].end(); it++) {
                    receivedSuccList.push_back(it->asUInt());
                }

                std::copy(receivedSuccList.begin(), receivedSuccList.end() - 2,
                        successorList.begin() + 2);
            }
            notify(successorList[0]);
        }

        // update replicas
        for (auto elem : succ["data"]) {
            unsigned long id = elem["id"].asUInt();
            string value = elem["value"].asString();
            if (inAB(id, predecessor, chordId)) {
                PlayerState ps;
                ps.deserialize(value);
                p_state[id] = ps;
            }
        }
    } else if (label.find("finger") != string::npos) {
        vector<string> parts;
        util::splitString(label, " ", parts);
        int index = util::strToInt(parts[1]);
        fingerTable[index] = succ["chordId"].asUInt();
    }
    delete msg;
}

void ChordCtrl::onNotify(ChordMessage* msg) {
    string content = msg->getContent();
    unsigned long nodeId = util::strToLong(content);
    if (predecessor == MAXid + 1
            || (inAB(nodeId, predecessor, this->chordId) && nodeId != chordId)
//            || !GlobalNodeListAccess().get()->isUp(predecessor)
            ) {
        predecessor = nodeId;

//        cout << fullName << " [" << chordId << "] update predecessor" << endl;
    }
    delete msg;
}

void ChordCtrl::onStore(ChordMessage* msg) {
    unsigned long id = msg->getKey();
    string content = msg->getContent();
    PlayerState ps;
    ps.deserialize(content);
    // only store the last version
    PlayerState old_ps = p_state[id];
    if (old_ps.version < ps.version) {
        p_state[id] = ps;

        // replicate value to successors
        int r = GlobalParametersAccess().get()->getChordReplicaSize();
        for (int i = 0; i < r; i++) {
            ChordMessage* replicate = new ChordMessage(msg::CHORD_REPLICATE);
            replicate->setSender(chordId);
            replicate->setContent(content.c_str());
            replicate->setLabel(msg->getLabel());
            replicate->setByteLength(content.length());
            r_transmit(replicate, successorList[i]);
        }

        dout_ch << fullName << " stored value " << content << " with Chord ID "
                << chordId << endl;
        cout << fullName << " stored value " << content << " with Chord ID "
                << chordId << endl;
    }

    ChordMessage* reply = new ChordMessage(msg::CHORD_REPLY);
    reply->setType(ChordMsgType::CHORD_STORE_REPLY);
    reply->setKey(id);
    reply->setContent("");
    reply->setSender(chordId);
    reply->setLabel(msg->getLabel());
    reply->setHop(msg->getHop() + 1);
    r_transmit(reply, msg->getSender());

    delete msg;
}

void ChordCtrl::onGet(ChordMessage* msg) {
    string target = msg->getContent();
    unsigned long id = msg->getKey();

    if (p_state.count(id) > 0) {
        ChordMessage* reply = new ChordMessage(msg::CHORD_REPLY);
        reply->setType(ChordMsgType::CHORD_GET_REPLY);
        string serialized = p_state[id].serialize();
        reply->setKey(id);
        reply->setContent(serialized.c_str());
        reply->setSender(chordId);
        reply->setLabel(msg->getLabel());
        reply->setHop(msg->getHop() + 1);
        reply->setByteLength(serialized.length());
        r_transmit(reply, msg->getSender());

        cout << simTime() << " " << chordId << " return GET result: "
                << p_state[id] << endl;

    } else {
        handleFailure(msg, ChordMsgType::CHORD_GET_REPLY);
        cout << simTime() << " " << chordId << ": " << data::DATA_EMPTY
                << " for id " << id << endl;
    }

    delete msg;
}

void ChordCtrl::handleFailure(ChordMessage* msg, int replyType) {
    if (msg->getType() == ChordMsgType::CHORD_GET) {
        string content = msg->getContent();
        ChordMessage* replyMsg = new ChordMessage(msg::CHORD_REPLY);
        replyMsg->setKey(msg->getKey());
        replyMsg->setType(replyType);
        replyMsg->setContent(data::DATA_EMPTY);
        replyMsg->setSender(chordId);
        replyMsg->setLabel(msg->getLabel());
        replyMsg->setHop(msg->getHop());
        replyMsg->setByteLength(strlen(data::DATA_EMPTY));
        r_transmit(replyMsg, msg->getSender());

        cout << simTime() << " " << chordId << " return DATA_EMPTY" << endl;
    }
}

void ChordCtrl::onReplicate(ChordMessage* msg) {
    unsigned long id = msg->getKey();
    string content = msg->getContent();
    PlayerState ps;
    ps.deserialize(content);
    // only store the last version
    PlayerState old_ps = p_state[id];
    if (old_ps.version < ps.version) {
        p_state[id] = ps;
    }

    delete msg;
}

void ChordCtrl::onFixReplicas(ChordMessage* msg) {
    string content = msg->getContent();
    Reader reader;
    Value entries;
    reader.parse(content, entries);

    for (auto elem : entries["data"]) {
        unsigned long id = elem["id"].asUInt();
        string value = elem["value"].asString();
        PlayerState ps;
        ps.deserialize(value);
        p_state[id] = ps;
    }

    delete msg;
}

void ChordCtrl::startMaint() {
//    scheduleAt(simTime() + maintain_cycle, maintainer);
}

void ChordCtrl::maintain(cMessage *msg) {
//    stabilize();
//    fixFingers();
//    fixReplicas();
//    scheduleAt(simTime() + maintain_cycle, maintainer);
}

void ChordCtrl::stabilize() {
    for (auto elem : successorList) {
        if (elem != MAXid + 1 && GlobalNodeListAccess().get()->isUp(elem)) {
            successorList[0] = elem;
            findSuccessor(elem, elem, "successor stabilize");
            return;
        }
    }
    EV << "All successors of node " << this->chordId << " are down!" << endl;
}

void ChordCtrl::fixFingers() {
    if (fingerToFix >= M)
        fingerToFix = 0;

    unsigned long id = (unsigned long) (chordId
            + (unsigned long) pow(2, fingerToFix)) % (unsigned long) pow(2, M);

    findSuccessor(chordId, id, "finger " + to_string(fingerToFix));

//    cout << simTime() << " fix finger " << fingerToFix << " on " << chordId
//            << " for id " << id << endl;

    fingerToFix++;
}

void ChordCtrl::fixReplicas() {
    Value entries;
    for (auto elem : p_state) {
        unsigned long id = elem.first;
        if (inAB(id, predecessor, chordId)) {
            string value = elem.second.serialize();
            Value entry;
            entry["id"] = (unsigned int) id;
            entry["value"] = value;
            entries["data"].append(entry);
        }
    }

    // replicate value to successors
    FastWriter fw;
    string s_entries = fw.write(entries);
    if (s_entries.find("data") < s_entries.length()) {
        int r = GlobalParametersAccess().get()->getChordReplicaSize();
        for (int i = 0; i < r; i++) {
            ChordMessage* replicate = new ChordMessage(msg::CHORD_FIX_REPLICA);
            replicate->setSender(chordId);
            replicate->setType(ChordMsgType::CHORD_FIX_REPLICA);
            replicate->setContent(s_entries.c_str());
            r_transmit(replicate, successorList[i]);
        }

//        cout << fullName << " [" << chordId << "] fix replicas" << endl;
    }
}

void ChordCtrl::join(unsigned long bootstrap) {
    // add the macro for context switch
    Enter_Method_Silent();

//    cout << simTime() << " add node: " << chordId << endl;

    findSuccessor(bootstrap, chordId, "successor first");
    for (int i = 0; i < M; i++) {
        unsigned long a = (unsigned long) (chordId + (unsigned long) pow(2, i))
        % (unsigned long) pow(2, M);
        findSuccessor(bootstrap, a, "finger " + to_string(i));
    }
}

void ChordCtrl::findSuccessor(unsigned long nodeToAsk, unsigned long id,
        string label) {
    ChordMessage* predmsg = new ChordMessage(msg::CHORD_SUCCESSOR);
    predmsg->setType(ChordMsgType::SUCCESSOR);
    predmsg->setHop(0);
    predmsg->setKey(id);
    predmsg->setContent(util::longToStr(id).c_str());
    predmsg->setLabel(label.c_str());
    predmsg->setSender(chordId);

//    cout << simTime() << " look up node: " << id << " on " << chordId
//            << " label: '" << label << "' nodeToAsk: " << nodeToAsk << endl;

    r_transmit(predmsg, nodeToAsk);
}

unsigned long ChordCtrl::closestPrecedingNode(unsigned long id) {
    vector<unsigned long> fullTable = getFullTable();
    unsigned long found = MAXid + 1;
    // equivalent to the last node in fullTable preceding the given ID
    for (int i = fullTable.size() - 1; i >= 0; i--) {
        unsigned long entry = fullTable[i];
        if (entry != MAXid + 1 && GlobalNodeListAccess().get()->isUp(entry)
                && inAB(entry, this->chordId, id)) {
            found = entry;
            break;
        }
    }

    // if the first preceding node is not found, search the id through one-to-one hop
    if (found == MAXid + 1) {
        for (auto elem : successorList) {
            if (GlobalNodeListAccess().get()->isUp(elem)) {
                found = elem;
                break;
            }
        }
    }

    return found;
}

vector<unsigned long> ChordCtrl::getFullTable() {
    set<unsigned long> temp(fingerTable.begin(), fingerTable.end());
    std::copy(successorList.begin(), successorList.end(),
            std::inserter(temp, temp.end()));
    vector<unsigned long> fullTable(temp.begin(), temp.end());

    std::sort(fullTable.begin(), fullTable.end(), Comparator(*this));

    fullTable.push_back(predecessor);
    // remove the chordId from the fullTable
    fullTable.erase(std::remove(fullTable.begin(), fullTable.end(), chordId),
            fullTable.end());
    return fullTable;
}

void ChordCtrl::notify(unsigned long nodeId) {
    ChordMessage* notifyMsg = new ChordMessage(msg::CHORD_NOTIFY);
    notifyMsg->setType(ChordMsgType::NOTIFY);
    notifyMsg->setContent(util::longToStr(chordId).c_str());
    notifyMsg->setSender(chordId);
    r_transmit(notifyMsg, nodeId);

//    cout << fullName << " [" << chordId
//            << "] notify successor for predecessor update" << endl;
}

bool ChordCtrl::inAB(unsigned long id, unsigned long a, unsigned long b) {
    if (id == a || id == b)
        return true;

    if (id > a && id < b)
        return true;
    if (id < a && a > b && id < b)
        return true;

    if (id > b && a > b && id > a)
        return true;

    return false;
}

Value ChordCtrl::profile() {
    Value profile;
    profile["predecessor"] = (unsigned int) predecessor;
    profile["chordId"] = (unsigned int) chordId;
    for (auto elem : successorList) {
        profile["successorList"].append((unsigned int) elem);
    }
    for (auto elem : p_state) {
        unsigned long id = elem.first;
        if (inAB(id, predecessor, chordId)) {
            string value = elem.second.serialize();
            Value entry;
            entry["id"] = (unsigned int) id;
            entry["value"] = value;
            profile["data"].append(entry);
        }
    }
    return profile;
}
