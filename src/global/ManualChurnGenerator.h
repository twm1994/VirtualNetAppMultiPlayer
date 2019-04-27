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

#ifndef GLOBAL_MANUALCHURNGENERATOR_H_
#define GLOBAL_MANUALCHURNGENERATOR_H_

#include <string>
#include "ChurnGenerator.h"
#include "../common/Constants.h"
#include "../objects/IPvXAddress.h"
#include "UnderlayConfigurator.h"
#include "UnderlayConfiguratorAccess.h"

using namespace std;

class ManualChurnGenerator: public ChurnGenerator {
private:
    simtime_t scan_cycle;
    cMessage* scan;
    string host;
protected:
    void initialize();
    void handleMessage(cMessage *msg);
public:
    ManualChurnGenerator();
    virtual ~ManualChurnGenerator();
    virtual simtime_t getSessionLength();
};

#endif /* GLOBAL_MANUALCHURNGENERATOR_H_ */
