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

/**
 * @file GlobalNodeList.cc
 * @author Markus Mauch, Robert Palmer, Ingmar Baumgart
 */

#include <iostream>
#include "GlobalNodeList.h"
#include "../common/MiscWatch.h"

Define_Module(GlobalNodeList);

std::ostream& operator<<(std::ostream& os, const BootstrapEntry& entry) {
    os << " " << *(entry.info);

    return os;
}

void GlobalNodeList::initialize() {
    WATCH_UNORDERED_MAP(peerStorage.getPeerHashMap());

    WATCH_MAP(chords);
}

void GlobalNodeList::handleMessage(cMessage* msg) {

}

void GlobalNodeList::addPeer(const IPvXAddress& ip, PeerInfo* info) {
    BootstrapEntry temp;
    temp.info = info;

    peerStorage.insert(std::make_pair(ip, temp));

    if (dynamic_cast<ChordInfo*>(info) != nullptr) {
        chords.insert( { dynamic_cast<ChordInfo*>(info)->getChordId(), ip });
        states.insert( { dynamic_cast<ChordInfo*>(info)->getChordId(), false });
    } else if (dynamic_cast<HostInfo*>(info) != nullptr) {
        hosts.insert( { dynamic_cast<HostInfo*>(info)->getHostId(), ip });
    }
}

void GlobalNodeList::killPeer(const IPvXAddress& ip) {
    PeerHashMap::iterator it = peerStorage.find(ip);

    if (it != peerStorage.end()) {

        if (dynamic_cast<ChordInfo*>(it->second.info) != nullptr) {
            unsigned long idToKill =
                    dynamic_cast<ChordInfo*>(it->second.info)->getChordId();
            chords.erase(idToKill);
            states.erase(idToKill);
        }

        peerStorage.erase(it);
    }
}

PeerInfo* GlobalNodeList::getPeerInfo(const TransportAddress& peer) {
    return getPeerInfo(peer.getIp());
}

PeerInfo* GlobalNodeList::getPeerInfo(const IPvXAddress& ip) {
    PeerHashMap::iterator it = peerStorage.find(ip);

    if (it == peerStorage.end())
        return NULL;
    else
        return it->second.info;
}

std::vector<IPvXAddress>* GlobalNodeList::getAllIps() {
    std::vector<IPvXAddress>* ips = new std::vector<IPvXAddress>;

    const PeerHashMap::iterator it = peerStorage.begin();

    while (it != peerStorage.end()) {
        ips->push_back(it->first);
    }

    return ips;
}

// Chord functions =====================================

bool GlobalNodeList::isUp(unsigned long chordId) {
    if (chords.count(chordId) > 0) {
        return true;
    }
    return false;
}

bool GlobalNodeList::isReady(unsigned long chordId) {
    if (chords.count(chordId) > 0 && states[chordId]) {
        return true;
    }
    return false;
}

void GlobalNodeList::ready(unsigned long chordId) {
    states[chordId] = true;
}

IPvXAddress GlobalNodeList::getNodeAddr(unsigned long id) {
    IPvXAddress addr;
    if (chords.count(id) > 0) {
        addr = chords[id];
    } else if (hosts.count(id) > 0) {
        addr = hosts[id];
    }
    return addr;
}

int GlobalNodeList::chordSize() {
    return chords.size();
}

ChordInfo* GlobalNodeList::randChord() {
    // Retrieve all keys
    vector<unsigned long> keys;
    for (auto elem : chords)
        keys.push_back(elem.first);
    int random = (int) uniform(0, keys.size());
    unsigned long key = keys[random];
    IPvXAddress addr = chords[key];
    ChordInfo* info = dynamic_cast<ChordInfo*>(getPeerInfo(addr));
    return info;
}
