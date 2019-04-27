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

#ifndef __MULTINODEFASTSMRSIMULATOIN_GLOBALSTATISTICS_H_
#define __MULTINODEFASTSMRSIMULATOIN_GLOBALSTATISTICS_H_

#include <omnetpp.h>
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"

using namespace std;
using namespace omnetpp;
using namespace boost;
using namespace boost::tuples;

/**
 * TODO - Generated class
 */
class GlobalStatistics: public cSimpleModule {
private:
    simtime_t coolDown;
    int consensus_start;
    int consensus_end;
    int leader_election_start;
    int leader_election_end;
    int reconfiguration_start;
    int reconfiguration_end;
    /*
     * record the sequence of sent events,
     * in the form of ((event_sender_name, logical_computer_name), (event_sequence number, event_sent_time))
     */
    std::map<boost::tuple<std::string, std::string>,
            std::set<boost::tuple<int, simtime_t> > > event_sent;
    // record the sequence of received events, in the form of (event_sender_name, logical_computer_name, event_sequence number)
    std::map<boost::tuple<std::string, std::string>, std::set<int> > event_received;

    double calculateDeliveryRate();
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
public:
    GlobalStatistics();
    void increaseConsensusStart();
    void increaseConsensusEnd();
    void increaseLeaderElectionStart();
    void increaseLeaderElectionEnd();
    void increaseReconfigurationStart();
    void increaseReconfigurationEnd();
    void addSentEvent(std::string senderName, std::string LCName, int sequence,
            simtime_t sent);
    void addReceivedEvent(std::string senderName, std::string LCName,
            int sequence);
};

#endif
