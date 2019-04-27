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

#ifndef OBJECTS_PLAYERSTATE_H_
#define OBJECTS_PLAYERSTATE_H_

#include <string>
#include <sstream>
#include "../common/Util.h"

using namespace std;

class PlayerState {
public:
    // storage
    unsigned short peerId;
    unsigned long version;
    vector<float> position;
    unsigned short skin;
    unsigned short gear;

    PlayerState() :
            peerId(0), version(0), position(3), skin(1), gear(0) {
        ;
    }
    virtual ~PlayerState() {
        ;
    }
    string serialize() const;
    void deserialize(string data);
    friend std::ostream& operator<<(std::ostream& os, const PlayerState state);
};

#endif /* OBJECTS_PLAYERSTATE_H_ */
