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

#include "Coordinator.h"

Define_Module(Coordinator);

#include <iostream>
#include <fstream>
std::ofstream dfile_co("VirtualNet_Coordinator.txt");
std::ostream dout_co(dfile_co.rdbuf());

Coordinator::Coordinator() {
    STAGE_NUM = 1;
}

Coordinator::~Coordinator() {
    // TODO Auto-generated destructor stub
}

int Coordinator::numInitStages() const {
    return STAGE_NUM;
}

void Coordinator::initialize(int stage) {
    locXMax = par("locXMax");
    locYMax = par("locYMax");
    locXMin = par("locXMin");
    locYMin = par("locYMin");

    LCXMax = par("LCXMax");
    LCYMax = par("LCYMax");
    LCXMin = par("LCXMin");
    LCYMin = par("LCYMin");

    radius = par("percept_radius");

    WATCH_MAP(clients);

    HostBase::initialize();

    // draw map border
    cFigure::Point upperLeft((double) locXMin, (double) locYMin);
    cFigure::Point upperRight((double) locXMax, (double) locYMin);
    cFigure::Point lowerLeft((double) locXMin, (double) locYMax);
    cFigure::Point lowerRight((double) locXMax, (double) locYMax);
    auto line = new cLineFigure("line1");
    line->setStart(upperLeft);
    line->setEnd(upperRight);
    line->setLineColor(cFigure::BLUE);
    line->setLineWidth(2);
    line->setVisible(true);
    getParentModule()->getCanvas()->addFigure(line);
    line = new cLineFigure("line2");
    line->setStart(upperLeft);
    line->setEnd(lowerLeft);
    line->setLineColor(cFigure::BLUE);
    line->setLineWidth(2);
    line->setVisible(true);
    getParentModule()->getCanvas()->addFigure(line);
    line = new cLineFigure("line3");
    line->setStart(lowerLeft);
    line->setEnd(lowerRight);
    line->setLineColor(cFigure::BLUE);
    line->setLineWidth(2);
    line->setVisible(true);
    getParentModule()->getCanvas()->addFigure(line);
    line = new cLineFigure("line4");
    line->setStart(lowerRight);
    line->setEnd(upperRight);
    line->setLineColor(cFigure::BLUE);
    line->setLineWidth(2);
    line->setVisible(true);
    getParentModule()->getCanvas()->addFigure(line);
}

void Coordinator::dispatchHandler(cMessage *msg) {
    // not used
}

ClientInfo& Coordinator::addClient(string clientName) {
    long x = (long) uniform((double) locXMin, (double) locXMax);
    long y = (long) uniform((double) locYMin, (double) locYMax);

    ClientInfo info(clientName, x, y);
    clients[clientName] = info;
    return clients[clientName];
}

void Coordinator::deleteClient(string clientName) {
    clients.erase(clientName);
}

void Coordinator::setRendezvous(string clientName, IPvXAddress rendezvous) {
    if (clients.count(clientName) == 0) {
        addClient(clientName);
    }

    clients[clientName].setRendezvous(rendezvous);
}

void Coordinator::setClientAddress(string clientName, IPvXAddress address) {
    if (clients.count(clientName) == 0) {
        addClient(clientName);
    }

    clients[clientName].setRendezvous(address);
}

ClientInfo& Coordinator::getClientInfo(string clientName) {
    if (clients.count(clientName) == 0) {
        addClient(clientName);
    }
    return clients[clientName];
}

set<ClientInfo> Coordinator::getNeighbors(string clientName, long x, long y) {
    set<ClientInfo> neighbors;
    for (map<string, ClientInfo>::iterator it = clients.begin();
            it != clients.end(); it++) {
        ClientInfo client = it->second;
        long clientX = client.getX();
        long clientY = client.getY();
        // exclude the caller itself
        if (clientName.compare(client.getName()) != 0) {
            double distance = sqrt(
                    pow((double) (x - clientX), 2)
                            + pow((double) (y - clientY), 2));
            if (distance <= (double) radius) {
                neighbors.insert(client);
            }
        }
    }
    return neighbors;
}

void Coordinator::updatePosition(string clientName, long x, long y,
        IPvXAddress addr, string LCName, set<string> neighbors,
        set<IPvXAddress> addrs) {
    // add the macro for context switch
    Enter_Method_Silent();

    // update position
    if(clients.count(clientName) == 0) {
        clients[clientName] = ClientInfo(clientName, x, y, addr);
    } else {
        clients[clientName].setX(x);
        clients[clientName].setY(y);
    }

    // to simplify simulation, only NPCs initialize communication
    if(clientName.find("NPC") < clientName.length()) {
        return;
    }

// search neighbors
    for (map<string, ClientInfo>::iterator it = clients.begin();
    it != clients.end(); it++) {
        ClientInfo client = it->second;
        long clientX = client.getX();
        long clientY = client.getY();
        // exclude the caller itself
        if(clientName.compare(client.getName()) != 0) {
            double distance = sqrt(
            pow((double) (x - clientX), 2)
            + pow((double) (y - clientY), 2));
            if (distance <= (double) radius && neighbors.count(client.getName()) == 0) {

//                // select a client to notify
//                if(caller.getName().compare(client.getName()) >= 0) {
//                    continue;
//                }

                dout_co << simTime() << " broadcast NEIGHBOR_ADD" << endl;

// add a new neighbor
                AddNeighbor* addNeighbor = new AddNeighbor(msg::NEIGHBOR_ADD);
                addNeighbor->setNeighbor(LCName.c_str());
                stringstream sstream;
                for (auto addr : addrs) {
                    string addrStr = addr.get4().str();
                    sstream << addrStr << ";";
                }
                string addrs = sstream.str();

//                cout << "addrs: " << addrs << endl;

                addNeighbor->setHostAddrs(addrs.c_str());
// add control information
                UDPControlInfo* udpControlInfo = new UDPControlInfo();
                udpControlInfo->setDestAddr(client.getRendezvous());

//                cout << "neighbor address: " << client.getRendezvous() << endl;

                addNeighbor->setControlInfo(udpControlInfo);
                sendFixedDelay(addNeighbor);
            } else if(distance > (double) radius && neighbors.count(client.getName()) > 0) {

//                // select a client to notify
//                if(caller.getName().compare(client.getName()) >= 0) {
//                    continue;
//                }

                dout_co << simTime() << " broadcast NEIGHBOR_RM" << endl;

// remove a new neighbor
                RemoveNeighbor* rmNeighbor = new RemoveNeighbor(msg::NEIGHBOR_RM);
                rmNeighbor->setNeighbor(LCName.c_str());
                stringstream sstream;
                for (auto addr : addrs) {
                    string addrStr = addr.get4().str();
                    sstream << addrStr << ";";
                }
                string addrs = sstream.str();
                rmNeighbor->setHostAddrs(client.getRendezvous().str().c_str());
                // add control information
                UDPControlInfo* udpControlInfo = new UDPControlInfo();
                udpControlInfo->setDestAddr(client.getRendezvous());
                rmNeighbor->setControlInfo(udpControlInfo);
                sendFixedDelay(rmNeighbor);
            }
        }
    }
}

Coordinate Coordinator::mapLocation(Coordinate c) {
    Coordinate ml;
    ml.x = (long) ((double) c.x + locXMin);
    ml.y = (long) (locYMax - (double) c.y);
    return ml;
}

Coordinate Coordinator::mapLocationBack(Coordinate c) {
    Coordinate ml;
    ml.x = (long) ((double) c.x - locXMin);
    ml.y = (long) (locYMax - (double) c.y);
    return ml;
}

long Coordinator::random_LC_X() {
    return LCXMin;
}

long Coordinator::random_LC_Y() {
    return (long) uniform((double) LCYMin, (double) LCYMax);
}

