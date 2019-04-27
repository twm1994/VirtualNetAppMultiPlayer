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

#ifndef __MULTINODEFASTSMRSIMULATOIN_REPLICANUMBERPOLICY_H_
#define __MULTINODEFASTSMRSIMULATOIN_REPLICANUMBERPOLICY_H_

#include <omnetpp.h>

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class ReplicaNumPolicy: public cSimpleModule {
private:
    int threshold;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
public:
    virtual int getThreshold();
    virtual int maxReplicNumber() {
        // nothing to do in the parent class
        return 0;
    }
};

#endif
