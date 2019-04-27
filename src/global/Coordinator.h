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

#ifndef COORDINATOR_H_
#define COORDINATOR_H_

#include <cmath>
#include <set>
#include <sstream>
#include <omnetpp.h>
#include "../common/HostBase.h"
#include "../objects/ClientInfo.h"
#include "../objects/IPvXAddress.h"
#include "../objects/Coordinate.h"
#include "../messages/AddNeighbor_m.h"
#include "../messages/RemoveNeighbor_m.h"

using namespace std;
using namespace omnetpp;

// a special host
class Coordinator: public HostBase {
public:
    Coordinator();
    virtual ~Coordinator();
    ClientInfo& addClient(string clientName);
    void deleteClient(string clientName);
    ClientInfo& getClientInfo(string clientName);
    void setRendezvous(string clientName, IPvXAddress rendezvous);
    void setClientAddress(string clientName, IPvXAddress address);
    set<ClientInfo> getNeighbors(string clientName, long x, long y);

    void updatePosition(string clientName, long x, long y, IPvXAddress addr,
            string LCName, set<string> neighbors, set<IPvXAddress> addrs);
    // convert the coordinate of game to the coordinate of the simulation in OMNeT++
    Coordinate mapLocation(Coordinate c);
    Coordinate mapLocationBack(Coordinate c);

    // randomly determine the location of a logical computer
    long random_LC_X();
    long random_LC_Y();

protected:
    int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage* msg);

private:
    int STAGE_NUM;
    long locXMax;
    long locYMax;
    long locXMin;
    long locYMin;

    long LCXMax;
    long LCYMax;
    long LCXMin;
    long LCYMin;

    // percept radius
    long radius;

    // maintain the mapping of client and their locations
    map<string, ClientInfo> clients;
};

#endif /* COORDINATOR_H_ */
