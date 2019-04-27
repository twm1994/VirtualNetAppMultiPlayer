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

#ifndef __MULTINODEFASTSMRSIMULATION_CLOCK_H_
#define __MULTINODEFASTSMRSIMULATION_CLOCK_H_

#include <omnetpp.h>
//#include "boost/tuple/tuple.hpp"
//#include "boost/tuple/tuple_comparison.hpp"
//#include "boost/tuple/tuple_io.hpp"

using namespace std;
using namespace omnetpp;
//using namespace boost::tuples;

/**
 * TODO - Generated class
 */
class Clock: public cSimpleModule {
private:
//    tuple<simtime_t, simtime_t> clock;
//    NetworkTime clock;
    simtime_t real;
    simtime_t observed;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
public:
    Clock() {
        observed = real = 0;
    }
    virtual simtime_t now();
    void setClock();
    /**
     * error = observed - real; observed = real + error
     */
    simtime_t getError();
};

#endif
