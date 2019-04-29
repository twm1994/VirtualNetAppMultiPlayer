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

#ifndef __VIRTUALNETAPP_CLIENTAPP_H_
#define __VIRTUALNETAPP_CLIENTAPP_H_

#include <omnetpp.h>

#include "../common/HostBase.h"
#include "../common/Constants.h"

#include "../global/CoordinatorAccess.h"

#include "../thread/jmutex.h"
#include "../thread/jmutexautolock.h"
#include "../thread/jthread.h"

#include "../server/connection.h"
#include "../server/environment.h"
#include "../server/utility.h"
#include "../server/clientserver.h"
#include "../server/map.h"

#include "../objects/Coordinate.h"

#include "../messages/AppData_m.h"
#include "../messages/ClientData_m.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep_ms(x) Sleep(x)
#else
#include <unistd.h>
#define sleep_ms(x) usleep(x*1000)
#endif

using namespace std;
using namespace omnetpp;

using namespace jthread;

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

class ClientCtrl;

/**
 * TODO - Generated class
 */
class ClientApp: public HostBase {
private:
    ClientCtrl* ctrl;
    // CS communication loop
    cMessage* commLoop;
    JMutex m_env_mutex;
    con::Connection m_con;
    JMutex m_con_mutex;
    float m_step_dtime;
    JMutex m_step_dtime_mutex;
    u16 peer_id;
    simtime_t comm_loop_timer;

    int STAGE_NUM;

    // for simulation visualization
    int perception;
    // oval-shape for search range
    cOvalFigure* circle;

    set<std::string> neighbors;
    bool initPosition;

    // client-server communication handling
    void Receive();
    void start(unsigned short port);
    bool checkExit(u8 *data, u32 datasize);
    bool checkCollapse(u8 *data, u32 datasize);
    bool updatePosition(u8 *data, u32 datasize);
    void displayPosition(long x, long y);
public:
    unsigned short port;

    ClientApp();
    virtual ~ClientApp();
    unsigned short getPeerId() const {
        return this->peer_id;
    }
    void setPeerId(unsigned short peerId);
    void sendUpdateToClient(std::string content);
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void dispatchHandler(cMessage *msg);
    virtual void finish();
};

#endif
