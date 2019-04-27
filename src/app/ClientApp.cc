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

#include "ClientApp.h"
#include "../ctrl/ClientCtrl.h"

Define_Module(ClientApp);

#include <iostream>
#include <fstream>
std::ofstream dfile_ca("VirtualNet_ClientApp.txt");
std::ostream dout_ca(dfile_ca.rdbuf());

ClientApp::ClientApp() :
        m_con(PROTOCOL_ID, 512) {
    STAGE_NUM = stage::CLIENT_INIT;
//    stepTimer = NULL;
    commLoop = NULL;
    circle = nullptr;
    port = 40000;
    peer_id = PEER_ID_NEW;
    initPosition = false;
}

ClientApp::~ClientApp() {
    if (commLoop != NULL) {
        cancelAndDelete(commLoop);
    }
}

int ClientApp::numInitStages() const {
    return STAGE_NUM;
}

void ClientApp::initialize(int stage) {
    if (stage == 0) {
        HostBase::initialize();
        fullName = getParentModule()->getFullName();
        comm_loop_timer = par("comm_loop").doubleValue();
        perception = par("perception").intValue();
        // refer to the under layer control
        ctrl = check_and_cast<ClientCtrl*>(
                getParentModule()->getSubmodule("ctrl"));

        WATCH(peer_id);
        WATCH_SET(neighbors);
    }

    if (stage == 1) {
        commLoop = new cMessage(msg::CS_COMM_LOOP);
        scheduleAt(simTime(), commLoop);

        m_env_mutex.Init();
        m_con_mutex.Init();
        m_step_dtime_mutex.Init();
        m_step_dtime = 0.0;

        start(port);
    }
}

void ClientApp::finish() {
    if (circle != NULL) {
        getParentModule()->getParentModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }
}

void ClientApp::dispatchHandler(cMessage *msg) {
    if (msg->isName(msg::CS_COMM_LOOP)) {
        try {
            //std::cout<<"Running m_server->Receive()"<<std::endl;
            // receive data from client
            Receive();
        } catch (con::NoIncomingDataException &e) {
        } catch (std::exception &e) {
            std::cout << "CSCommThread: Some exception: " << e.what()
                    << std::endl;
            dout_ca << simTime() << " " << fullName
                    << " CSCommThread: Some exception: " << e.what()
                    << std::endl;
        }
        scheduleAt(simTime() + comm_loop_timer, commLoop);
    }
}

void ClientApp::start(unsigned short port) {
//    stop();
    m_con.setTimeoutMs(0);
    m_con.Serve(port);
    cout << "Server started" << endl;
}

void ClientApp::Receive() {
    u32 data_maxsize = 10000;
    Buffer<u8> data(data_maxsize);
    u32 datasize;
    // temporarily not used
    u16 pid;

    try {
        {
            JMutexAutoLock lock(m_con_mutex);
            datasize = m_con.Receive(pid, *data, data_maxsize);
        }
        if (datasize > 0) {

            std::string encoded;
            util::binary_to_string(*data, datasize, encoded);
            std::string content = encoded + "+" + std::to_string(datasize) + "+"
                    + std::to_string(peer_id);

//            dout_ca << fullName << " received client command: " << encoded
//                    << endl;

            if (updatePosition(*data, datasize)) {
                content = content + "+" + fullName;
            } else if (checkExit(*data, datasize)) {
                content = content + "|" + event::TO_EXIT;
                CoordinatorAccess().get()->deleteClient(fullName);
            }

            ctrl->cacheAppData(content);
        }
    } catch (con::InvalidIncomingDataException &e) {
        std::cout << "Server::Receive(): "
                "InvalidIncomingDataException: what()=" << e.what()
                << std::endl;
        dout_ca << simTime() << " " << fullName << " Server::Receive(): "
                "InvalidIncomingDataException: what()=" << e.what()
                << std::endl;
    } catch (con::NoIncomingDataException &e) {
        ;
    }
}

bool ClientApp::checkExit(u8 *data, u32 datasize) {
    try {
        if (datasize < 2)
            return false;
        ToServerCommand command = (ToServerCommand) readU16(&data[0]);
        if (command == TOSERVER_EXIT) {
            return true;
        }
    } catch (SendFailedException &e) {
        std::cout << "Server::ProcessData(): SendFailedException: " << "what="
                << e.what() << std::endl;
        dout_ca << simTime() << " " << fullName
                << " Server::ProcessData(): SendFailedException: " << "what="
                << e.what() << std::endl;
    }
    return false;
}

bool ClientApp::updatePosition(u8 *data, u32 datasize) {
    try {
        if (datasize < 2)
            return false;

        ToServerCommand command = (ToServerCommand) readU16(&data[0]);

        if (command == TOSERVER_PLAYERPOS) {
            if (datasize < 2 + 12 + 12)
                return false;

            Coordinator* coordinator = CoordinatorAccess().get();
            if (!initPosition) {
                initPosition = true;
                coordinator->addClient(fullName);
                coordinator->setClientAddress(fullName, ctrl->getIPAddress());
            }
            v3s32 ps = readV3S32(&data[2]);

//            cout << "new position: " << ps.X << ", " << ps.Z << endl;

            Coordinate position((long) ps.X / 100, (long) ps.Z / 100);
            Coordinate c = coordinator->mapLocation(position);
            displayPosition(c.x, c.y);
            coordinator->updatePosition(fullName, c.x, c.y,
                    ctrl->getIPAddress(), ctrl->getLCName(), neighbors,
                    ctrl->getMeshReplicas());
            return true;
        }
    } //try
    catch (SendFailedException &e) {
        std::cout << "Server::ProcessData(): SendFailedException: " << "what="
                << e.what() << std::endl;
        dout_ca << simTime() << " " << fullName
                << " Server::ProcessData(): SendFailedException: " << "what="
                << e.what() << std::endl;
    }
    return false;
}

void ClientApp::sendUpdateToClient(std::string content) {
    std::vector<std::string> container;
    util::splitString(content, "+", container);
    std::string encoded = container[0];
    unsigned int datasize = util::strToInt(container[1]);
    u8 channelnum = (u8) util::strToInt(container[2]);

//    cout << fullName << " ClientApp received update: " << content << endl;

    if (container.size() == 6) {
        std::string neighbor = container[5];

        if (container[4].compare(event::TO_JOIN) == 0
                && neighbor.compare(fullName) != 0) {

//            dout_ca << simTime() << " " << fullName
//                    << " neighbor join message from " << neighbor << endl;

            neighbors.insert(neighbor);
        } else if (container[4].compare(event::TO_REMOVE) == 0
                && neighbor.compare(fullName) != 0) {

//            dout_ca << simTime() << " " << fullName
//                    << " neighbor leave message from " << neighbor << endl;

            neighbors.erase(neighbor);
        }
    }
    unsigned char decoded[datasize];
    util::string_to_binary(encoded, decoded, datasize);
    SharedBuffer<u8> reply(decoded, datasize);

//    std::cout << fullName << " reply to client: " << encoded << std::endl;
//    std::cout << fullName << " peer_id: " << peer_id << std::endl;

    if (peer_id != PEER_ID_NEW) {
        m_con.Send(peer_id, channelnum, reply, true);
    }
}

void ClientApp::displayPosition(long x, long y) {
    getParentModule()->getDisplayString().setTagArg("p", 0, x);
    getParentModule()->getDisplayString().setTagArg("p", 1, y);

//    cout << "move client to: " << x << ", " << y << endl;

    // display the perception range
    if (circle != nullptr) {
        getParentModule()->getParentModule()->getCanvas()->removeFigure(circle);
        delete circle;
    }
    circle = new cOvalFigure("AoI");
    double mapRadius = perception;
    cout << "mapRadius: " << mapRadius << endl;
    circle->setBounds(
            cFigure::Rectangle((double) (x - mapRadius),
                    (double) (y - mapRadius), (double) (2 * mapRadius),
                    (double) (2 * mapRadius)));
    circle->setLineColor(cFigure::MAGENTA);
    circle->setLineWidth(2);
    circle->setLineStyle(cFigure::LINE_DOTTED);
    circle->setVisible(true);
    getParentModule()->getParentModule()->getCanvas()->addFigure(circle);
}

void ClientApp::setPeerId(unsigned short peerId) {
    this->peer_id = peerId;
    m_con.SetPeerID(peerId);
    ctrl->id = peerId;
}
