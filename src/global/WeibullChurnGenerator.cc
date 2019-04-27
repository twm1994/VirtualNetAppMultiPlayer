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

#include "WeibullChurnGenerator.h"

Define_Module(WeibullChurnGenerator);

void WeibullChurnGenerator::initialize()
{
    fraction = par("stable_fraction");
    stableMean = par("stable_mean");
    sessionLenShape = par("sessionLenShape");
    sessionLenScale = par("sessionLenScale");
    sessionLenMin = par("sessionLenMin");
}

void WeibullChurnGenerator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}

simtime_t WeibullChurnGenerator::getSessionLength(){
    simtime_t sessionLength = exponential(stableMean);
    if(uniform(0, 1) > fraction){
        // unit of minute
        sessionLength = weibull(sessionLenScale, sessionLenShape) * 60;
        // TODO hard-code here, may be removed later
        if(sessionLength < sessionLenMin){
            sessionLength = sessionLenMin;
        }
    }
    return sessionLength;
}
