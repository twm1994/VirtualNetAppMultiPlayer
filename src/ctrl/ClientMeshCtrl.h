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

#ifndef __VIRTUALNETAPP_CLIENTMESHCTRL_H_
#define __VIRTUALNETAPP_CLIENTMESHCTRL_H_

#include <omnetpp.h>
#include "../server/environment.h"
#include "../server/utility.h"
#include "../server/clientserver.h"
#include "../server/map.h"
#include "../server/npc.h"
#include "../common/HostBase.h"
#include "../common/sdbm.h"
#include "../common/PearsonHash.h"
#include "../common/Util.h"
#include "../global/UnderlayConfiguratorAccess.h"
#include "../global/CoordinatorAccess.h"
#include "../objects/Coordinate.h"
#include "../messages/AddNeighbor_m.h"
#include "../messages/RemoveNeighbor_m.h"
#include "../messages/HandShakeTimeout_m.h"
#include "../messages/HandShakeReject_m.h"
#include "../messages/HandShake_m.h"
#include "../messages/ClientCycleTimeout_m.h"
#include "../messages/Join_m.h"
#include "../messages/Event_m.h"
#include "../messages/ConfigUpdate_m.h"

using namespace std;
using namespace omnetpp;

/**
 * TODO - Generated class
 */
class ClientMeshCtrl: public HostBase {
private:
    unsigned long id;
    unsigned short peerId;
    int STAGE_NUM;

    // event sequences to different logical computers, in (logical_computer_name, event_seq)
    map<string, int> seq;

    // replicas of the neighbor users' logical computer, in (logical_computer_name, replica_address)
    multimap<string, IPvXAddress> replicaMaps;
    set<IPvXAddress> meshReplicas;
    multimap<string, IPvXAddress> neighborMaps;
    std::string LCName;
    set<std::string> neighbors;
    std::map<std::string, IPvXAddress> rendezvousMaps;

    set<string> neighborsToAdd;
    set<string> neighborsToRemove;

    // time related variables
    // length of a round
    simtime_t cycle;
    cMessage* updatePosition;

    map<string, ClientCycleTimeout*> timeouts;
    // (logical computer (name), start time)
    map<string, simtime_t> starts;
    simtime_t connectCycle;

    // simulation environment
    Environment m_env;
    unsigned short skin;
    unsigned short gear;

    void handleAddNeighbor(cMessage* msg);
    void handleRMNeighbor(cMessage* msg);
    void handleHandShakeReject(cMessage* msg);
    void handleHandShakeTimeout(cMessage* msg);
    void handleJoin(cMessage* msg);
    void handleCycleEvent(cMessage* msg);
    string getPlayerPosition();
    void updatePlayerPosition(cMessage* msg);
    void displayPosition(long x, long y);
    void handleConfigUpdate(cMessage* msg);
    void initMap(v3f position);
    void disposeTimeouts();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage *msg);
    virtual void finish();
public:
    ClientMeshCtrl();
    virtual ~ClientMeshCtrl();
};

#endif
