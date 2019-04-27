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

#include "Clock.h"

Define_Module(Clock);

void Clock::initialize() {
    WATCH(real);
    WATCH(observed);
}

void Clock::handleMessage(cMessage *msg) {
    // TODO - Generated method body
}

simtime_t Clock::now() {
    setClock();
    return observed;
}

void Clock::setClock() {
    // simulate the time of the root host in a NTP tree
    simtime_t root = simTime();
    if (root != real) {
        double mean = par("err_mean");
        double std = par("err_std");
        double sign = pow((double) -1, (int) bernoulli(0.5));
        simtime_t unsigned_error = truncnormal(mean, std);
        simtime_t error = sign * unsigned_error;
        real = root;
        observed = root + error;

        EV << "mean: " << mean << endl;
        EV << "std: " << std << endl;
        EV << "sign: " << sign << endl;
        EV << "unsigned_error: " << unsigned_error << endl;
        EV << "error: " << error << endl;
        EV << "real: " << real << "ms" << endl;
        EV << "observed: " << observed << "ms" << endl;
    }
}

simtime_t Clock::getError() {
    setClock();
    return observed - real;
}

