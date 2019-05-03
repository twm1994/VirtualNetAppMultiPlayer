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

#ifndef __FASTSMRSIMULATION_NODEAPP_H_
#define __FASTSMRSIMULATION_NODEAPP_H_

#include <map>
#include <vector>
#include <sstream>

#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"

#include "../server/environment.h"
#include "../server/utility.h"
#include "../server/clientserver.h"
#include "../server/map.h"

#include "../objects/PlayerState.h"
#include "../objects/HostInfo.h"

#include "../common/HostBase.h"
#include "../common/sdbm.h"
#include "../messages/EventApply_m.h"
#include "../messages/UpdateApply_m.h"
#include "../messages/ChordMessage_m.h"
#include "../messages/InitApp_m.h"

using namespace std;
using namespace omnetpp;
using namespace boost;
using namespace boost::tuples;

class NodeCtrl;

class NodeApp: public HostBase {
private:
    // applied event queue, tuple (event index, event content)
    map<int, string> Qa;
    // the corresponding keys of the applied events in Qa
    map<int, boost::tuple<int, unsigned long, int> > Qa_key;
    int lastApplied;
    // states in tuple (state #, state)
    map<int, unsigned long> Qs;
    unsigned long initial_state;
    int STAGE_NUM;

    simtime_t step_timer;
//    cMessage* step_msg;

// game states ===============================
    Environment m_env;
    unsigned short peer_id;
    vector<float> position;
    unsigned short skin;
    unsigned short gear;
    // state version
    unsigned long version;

    // ID on the P2P cloud
    unsigned long id;
    unsigned long reqId;

    // game state initialization
    bool initialized;
    bool init_empty;
    bool init_start;
    bool terminating;

    // cycle to persist app state
    cMessage* persist;
    simtime_t persist_cycle;
    NodeCtrl* ctrl;

    // first client event is expected to approach after the (configured) duration of client initialization
    void initClientState();
    void applyEvent(EventApply* event = NULL);
    std::string processData(u8 *data, u32 datasize, u16 peer_id = 0,
            std::string clinetId = "");
    bool isInitCommand(std::string event);
    std::string removeNeighbor(u16 peerId, std::string clinetId);

    // handle the message from the P2P cloud
    void persistAppState();
    void handleChordMsg(ChordMessage* chordMsg);

protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage *msg);
public:
    NodeApp();
    virtual ~NodeApp();
    void initState(unsigned long state);
    string packState();
    std::string packLastQState();
    void unpackState(string state);
    void unpackLastQState(std::string lastQState);
    void initApp(unsigned short peerId);
    void onExit();
};

#endif
