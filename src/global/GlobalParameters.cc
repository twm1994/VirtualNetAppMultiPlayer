//
// Copyright (C) 2007 Institut fuer Telematik, Universitaet Karlsruhe (TH)
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
 * @file GlobalParameters.cc
 * @author IngmarBaumgart
 */

#include <omnetpp.h>

#include "GlobalParameters.h"
#include "../server/map.h"

Define_Module(GlobalParameters);

void GlobalParameters::initialize() {
    maxDist = par("max_distance");
    fixedLatency = par("fix_latency");
    latencyMode = par("latency_gen_mode");

    spaceSize = par("address_space_size");
    succSize = par("successor_list_size");
    chord_init_size = par("chord_init_size");
    replicaSize = par("chord_replica_size");

    int map_length = par("map_length");
    int map_width = par("map_width");
    int map_height = par("map_height");
    int map_bottom = par("map_bottom");
    const char* map_file = par("map_file").stringValue();
    Map::MAP_LENGTH = map_length;
    Map::MAP_WIDTH = map_width;
    Map::MAP_HEIGHT = map_height;
    Map::MAP_BOTTOM = map_bottom;
    Map::SERVER_MAP_FILE = map_file;
}

double GlobalParameters::getDistance(IPvXAddress host1, IPvXAddress host2) {
    double distance = 0;
    boost::tuple<IPvXAddress, IPvXAddress> key(host1, host2);
    boost::tuple<IPvXAddress, IPvXAddress> mkey(host2, host1);
    if (distances.count(key) > 0) {
        distance = distances[key];
    } else if (distances.count(key) > 0) {
        distance = distances[mkey];
    } else {
        distance = uniform(0, maxDist);
        distances[key] = distance;
    }
    return distance;
}

void GlobalParameters::remoteHost(IPvXAddress host) {
    vector<boost::tuple<IPvXAddress, IPvXAddress> > toRemove;
    for (map<boost::tuple<IPvXAddress, IPvXAddress>, double>::iterator it =
            distances.begin(); it != distances.end(); it++) {
        boost::tuple<IPvXAddress, IPvXAddress> key = it->first;
        if (get<0>(key) == host || get<1>(key) == host) {
            toRemove.push_back(key);
        }
    }
    for (size_t i = 0; i < toRemove.size(); i++) {
        distances.erase(toRemove[i]);
    }
}

simtime_t GlobalParameters::getFixLatency() {
    return fixedLatency;
}

int GlobalParameters::getLatencyGenerationMode() {
    return latencyMode;
}

int GlobalParameters::getAddrSpaceSize() {
    return spaceSize;
}

int GlobalParameters::getSuccListSize() {
    return succSize;
}

int GlobalParameters::getChordInitSize() {
    return chord_init_size;
}

int GlobalParameters::getChordReplicaSize() {
    return replicaSize;
}
