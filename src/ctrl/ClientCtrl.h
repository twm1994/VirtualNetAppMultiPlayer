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

#ifndef CLIENT_CTRL_H_
#define CLIENT_CTRL_H_

#include <functional>
#include <map>
#include <queue>
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"

#include "../common/HostBase.h"
#include "../common/sdbm.h"
#include "../common/Util.h"
#include "../common/MiscWatch.h"
#include "../global/UnderlayConfiguratorAccess.h"
#include "../global/CoordinatorAccess.h"
#include "../global/GlobalStatisticsAccess.h"
#include "../objects/IPAddress.h"
#include "../objects/IPvXAddress.h"
#include "../objects/SimpleNodeEntry.h"
#include "../objects/SimpleInfo.h"
#include "../objects/TransportAddress.h"
#include "../objects/ClientInfo.h"
#include "../messages/Join_m.h"
#include "../messages/InitJoin_m.h"
#include "../messages/Event_m.h"
#include "../messages/Update_m.h"
#include "../messages/Timeout_m.h"
#include "../messages/Connect_m.h"
#include "../messages/ConnectReply_m.h"
#include "../messages/ConnectTimeout_m.h"
#include "../messages/ClientCycleTimeout_m.h"
#include "../messages/ConfigUpdate_m.h"
#include "../messages/AppData_m.h"
#include "../messages/ClientData_m.h"
#include "../messages/AddNeighbor_m.h"
#include "../messages/RemoveNeighbor_m.h"
#include "../messages/HandShakeTimeout_m.h"
#include "../messages/HandShakeReject_m.h"
#include "../messages/HandShake_m.h"

using namespace std;
using namespace omnetpp;
using namespace boost;
using namespace boost::tuples;

class ClientApp;

class ClientCtrl: public HostBase {
private:
    // event sequences to different logical computers, in (logical_computer_name, event_seq)
    std::map<std::string, int> seq;
    // tuple (sender Id, update index);
    std::map<unsigned long, int> senders;
    // tuple (event sender Id, event sequence, update content)
    std::map<boost::tuple<unsigned long, int>, std::string> trace;
    bool flag;
//    bool init;
    int STAGE_NUM;

    // remote addresses
    IPvXAddress rendezvous;
    // replicas of the neighbor users' logical computer, in (logical_computer_name, replica_address)
    multimap<std::string, IPvXAddress> replicaMaps;
    multimap<std::string, IPvXAddress> neighborMaps;
    /*
     * rendezvous of the neighbor users' logical computer, in (logical_computer_name, replica_address),
     * currently not used
     */
    std::map<std::string, IPvXAddress> rendezvousMaps;
    std::string LCName;

    // time related variables
    // current local clock, calculated by adding the volatile offset value
    simtime_t clock;
    // length of a round
    simtime_t cycle;
    // (logical computer (name), start time)
    std::map<std::string, simtime_t> starts;
    simtime_t discovery;
    simtime_t connectCycle;
    simtime_t loadCycle;
    simtime_t handshakeCycle;

    std::map<std::string, ClientCycleTimeout*> timeouts;
    cMessage* loadTimeout;
    cMessage* neighborDiscovery;

    // set of queues for different Meshes, in <LCName, queued_data>
    std::map<std::string, queue<std::string>> appDataQueue;

    std::set<std::string> neighborsToAdd;
    std::set<std::string> neighborsToRemove;

    ClientApp* app;

    /*
     * for result recording
     */
    simsignal_t interactionLatency;
    // record the time of sent events, in the form of ((sender_ID, logical_computer_name, event_sequence number), event sent time)
    std::map<boost::tuple<unsigned long, std::string, int>, simtime_t> sentEvents;

    void handleJoin(cMessage* msg);
    void handleCycleEvent(cMessage* msg);
    void handleUpdate(cMessage* msg);
    void handleConfigUpdate(cMessage* msg);
    void handleLoad();
    void handleConnectReply(cMessage* msg);
    void handleConnectTimeout(cMessage* msg);
    void handleNeighborDiscovery(cMessage* msg);
    void handleAddNeighbor(cMessage* msg);
    void handleRMNeighbor(cMessage* msg);
    void handleHandShakeReject(cMessage* msg);
    void handleHandShakeTimeout(cMessage* msg);

//    void handleExit(cMessage* msg);
    void disposeTimeouts();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage *msg);
    virtual void finish();
public:
    unsigned long id;

    ClientCtrl();
    virtual ~ClientCtrl();
    set<IPvXAddress> getMeshReplicas();
    std::string getLCName();
    IPvXAddress getRendezvous() {
        return rendezvous;
    }
    void cacheAppData(std::string userData);
};

#endif /* CLIENT_CTRL_H_ */
