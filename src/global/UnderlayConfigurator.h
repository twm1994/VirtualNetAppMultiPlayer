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

#ifndef _UNDERLAYCONFIGURATOR_H_
#define _UNDERLAYCONFIGURATOR_H_

#include <omnetpp.h>
#include "../common/HostBase.h"
#include "GlobalNodeListAccess.h"
#include "GlobalParametersAccess.h"
#include "ChurnGeneratorAccess.h"
#include "CoordinatorAccess.h"

#include "../objects/IPAddress.h"
#include "../objects/IPvXAddress.h"
#include "../objects/SimpleNodeEntry.h"
#include "../objects/SimpleInfo.h"
#include "../objects/TransportAddress.h"
#include "../objects/ClientInfo.h"
#include "../others/InterfaceTableAccess.h"

#include "../transport/NetworkAddress.h"

#include "../messages/Failure_m.h"
#include "../messages/Termination_m.h"

using namespace std;
using namespace omnetpp;

namespace con {
class Connection;
}

/**
 * Base class for configurators of different underlay models
 *
 * @author Stephan Krause, Bernhard Heep
 */
class UnderlayConfigurator: public cSimpleModule {
public:
    UnderlayConfigurator();
    virtual ~UnderlayConfigurator();
    TransportAddress* createLogicComputer();
    TransportAddress* createNode(cModule* parent, int nodeCreated);
    TransportAddress* registerEndpoint(cModule* host);
    void removeNode(IPvXAddress& nodeAddr);
    void removeHost(Termination* t);
    // remove all the connections to the Mesh; if toRemoveNode = true, the node  will be timed to remove
    void revokeNode(IPvXAddress& nodeAddr, bool toRemoveNode = false);
    // schedule to remove a Mesh
    void revokeLogicComputer(IPvXAddress& nodeAddr);
    void revokeClient(IPvXAddress& nodeAddr);
    void disposeFailures();
protected:
    /**
     * OMNeT number of init stages
     */
    int numInitStages() const;
    /**
     * OMNeT init methods
     */
    virtual void initialize(int stage);
    virtual void finish();
    /**
     * Node mobility simulation
     *
     * @param msg timer-message
     */
    virtual void handleMessage(cMessage* msg);

private:
    // logic computer creation counter
    int lcCreated;
    int clientCreated;
    int neighborCreated;
    int chord_counter;

    GlobalNodeList* globalNodeList;
    GlobalParameters* parameterList;
    Coordinator* coordinator;

    // record the time of death of each node, in (node_address, node_death_time)
    std::map<std::string, simtime_t> death_schedule;
    std::map<std::string, Failure*> failures;

    simtime_t loginLoop;
    simtime_t remove_wait;
    cMessage* loginMsg;
    con::Connection* login_con;

    int stopwatch;

    void handleNodeFailure(Failure* failure);
    void handleClientLogin(cMessage* msg);
    void initChordOverlay(cMessage* msg);
};

#endif
