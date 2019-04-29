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

#ifndef OTHERS_TIMEOUTSCHEDULER_H_
#define OTHERS_TIMEOUTSCHEDULER_H_

#include "../messages/HBTimeout_m.h"
#include "../messages/HBProbe_m.h"
#include "../messages/HBResponse_m.h"

using namespace std;
using namespace omnetpp;

class TimeoutScheduler {
private:

public:
    simtime_t timeout;
    HBTimeout* event;
    HBResponse* hbr;
    HBProbe* hb;
    TimeoutScheduler() {
        timeout = 0.0;
        event = NULL;
        hbr = NULL;
    }
    ~TimeoutScheduler() {
    }
};

#endif /* OTHERS_TIMEOUTSCHEDULER_H_ */
