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

#ifndef REND_H_
#define REND_H_

#include "../common/HostBase.h"
#include "../common/sdbm.h"
#include "../global/UnderlayConfiguratorAccess.h"
#include "../objects/IPAddress.h"
#include "../objects/IPvXAddress.h"
#include "../objects/SimpleNodeEntry.h"
#include "../objects/SimpleInfo.h"
#include "../objects/TransportAddress.h"
#include "../others/BasicReplicaNumPolicy.h"
#include "../others/ReplicaNumPolicy.h"
#include "../others/ReplicaNumPolicyAccess.h"
#include "../messages/UDPControlInfo_m.h"
#include "../messages/HBTimeout_m.h"
#include "../messages/HBResponse_m.h"
#include "../messages/HBProbe_m.h"
#include "../messages/Connect_m.h"
#include "../messages/ConnectReply_m.h"

using namespace std;
using namespace omnetpp;

class TimeoutScheduler {
private:

public:
    simtime_t timeout;
    HBTimeout* event;
    TimeoutScheduler() {
        timeout = 0.0;
        event = NULL;
    }
    ~TimeoutScheduler() {
    }
};

class Rendezvous: public HostBase {
private:
    // name of the logical computer
    string LCName;
    unsigned short peer_id;
    set<IPvXAddress> survivals;
    map<IPvXAddress, string> routingTable;
    // It is better to use pointer in map if the referenced object contains a pointer member.
    // The pointer should be released later.
    // This map cannot be assigned to WATCH_MAP, which will cause compilation error.
    map<string, TimeoutScheduler*> timeouts;
    simtime_t cycle;
    // host creation counter
    int hostCreated;
//    int groupSize;
    int STAGE_NUM;
    UnderlayConfigurator* configurator;

    // application termination
    set<IPvXAddress> toTerminates;

    void handleClientConnect(Connect* msg);
    void handleHeartbeatResp(HBResponse* hbr);
    void handleTimeout(HBTimeout* timeout);
    void handleTermination(cMessage* msg);

    void addNodes(int num, bool isInit);
    void disposeTimeouts();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage* msg);
public:
    Rendezvous();
    virtual ~Rendezvous();
};

#endif /* REND_H_ */
