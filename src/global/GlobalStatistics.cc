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

#include "GlobalStatistics.h"

Define_Module(GlobalStatistics);

GlobalStatistics::GlobalStatistics() {
    consensus_start = 0;
    consensus_end = 0;
    leader_election_start = 0;
    leader_election_end = 0;
    reconfiguration_start = 0;
    reconfiguration_end = 0;
}

void GlobalStatistics::initialize() {
    coolDown = par("cool_down");
    WATCH(consensus_start);
    WATCH(consensus_end);
    WATCH(leader_election_start);
    WATCH(leader_election_end);
    WATCH(reconfiguration_start);
    WATCH(reconfiguration_end);
}

void GlobalStatistics::handleMessage(cMessage *msg) {
    // TODO - Generated method body
}

void GlobalStatistics::increaseConsensusStart() {
    consensus_start++;
}

void GlobalStatistics::increaseConsensusEnd() {
    consensus_end++;
}

void GlobalStatistics::increaseLeaderElectionStart() {
    leader_election_start++;
}

void GlobalStatistics::increaseLeaderElectionEnd() {
    leader_election_end++;
}

void GlobalStatistics::increaseReconfigurationStart() {
    reconfiguration_start++;
}

void GlobalStatistics::increaseReconfigurationEnd() {
    reconfiguration_end++;
}

void GlobalStatistics::addSentEvent(string senderName, string LCName,
        int sequence, simtime_t sent) {
    boost::tuple<string, string> key(senderName, LCName);
    if (event_sent.count(key) > 0) {
        set<boost::tuple<int, simtime_t> >& eventList = event_sent[key];
        eventList.insert(boost::tuple<int, simtime_t>(sequence, sent));
    } else {
        set<boost::tuple<int, simtime_t> > eventList;
        eventList.insert(boost::tuple<int, simtime_t>(sequence, sent));
        event_sent[key] = eventList;
    }
}

void GlobalStatistics::addReceivedEvent(string senderName, string LCName,
        int sequence) {
    boost::tuple<string, string> key(senderName, LCName);
    if (event_received.count(key) > 0) {
        set<int>& eventList = event_received[key];
        eventList.insert(sequence);
    } else {
        set<int> eventList;
        eventList.insert(sequence);
        event_received[key] = eventList;
    }
}

void GlobalStatistics::finish() {
    cout << "Generating statistical results" << endl;
    recordScalar("Duration", simTime());
    recordScalar("Consensus start #", consensus_start);
    recordScalar("Consensus end #", consensus_end);
    // exclude the one in group initialization
    recordScalar("Leader election start #", leader_election_start - 1);
    // exclude the one in group initialization
    recordScalar("Leader election end #", leader_election_end - 1);
    recordScalar("reconfiguration start #", reconfiguration_start);
    recordScalar("reconfiguration end #", reconfiguration_end);
    recordScalar("Update delivery rate", calculateDeliveryRate());
    cout << "End of statistical results generation" << endl;
}

double GlobalStatistics::calculateDeliveryRate() {
    double total_delivery_rate = 0;
    int num_event_sent = 0;
    int num_event_received = 0;

    map<boost::tuple<string, string>, int> lastSequences;
    for (auto elem_rcvd : event_received) {
        set<int> seqRcveNumList = elem_rcvd.second;
        // find the last received sequence number;
        int lastSequence = *seqRcveNumList.rbegin();
        lastSequences[elem_rcvd.first] = lastSequence;
    }

    for (auto elem_sent : event_sent) {
        boost::tuple<string, string> eventKey = elem_sent.first;
        set<boost::tuple<int, simtime_t> > seqSentNumList = elem_sent.second;
        for (auto elem_seq : seqSentNumList) {
            int sequence = get<0>(elem_seq);
            simtime_t sent = get<1>(elem_seq);
            // only check events sent COOL_DOWN seconds ago
            simtime_t threshold = simTime() - coolDown;
            if ((event_received.count(eventKey) > 0
                    && sequence <= lastSequences[eventKey])
                    || sent <= threshold) {
                num_event_sent++;
                set<int> seqRcveNumList = event_received[eventKey];
                if (seqRcveNumList.count(sequence) > 0) {
                    num_event_received++;
                }
            }
        }
    }

    total_delivery_rate = num_event_received / (double) num_event_sent;
    return total_delivery_rate;
}
