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

#include "PlayerState.h"

//public

string PlayerState::serialize() const {
    stringstream ss;

    ss << peerId << "|" << version << "|" << position[0] << "-" << position[1]
            << "-" << position[2] << "|" << skin << "|" << gear;

    return ss.str();
}

void PlayerState::deserialize(string data) {
    vector<string> container;
    util::splitString(data, "|", container);
    if(container.size() == 5){
        this->peerId = util::strToUShort(container[0]);
        this->version = util::strToLong(container[1]);
        vector<string> subcontainer;
        util::splitString(container[2], "-", subcontainer);
        this->position[0] = (float) util::strToDouble(subcontainer[0]);
        this->position[1] = (float) util::strToDouble(subcontainer[1]);
        this->position[2] = (float) util::strToDouble(subcontainer[2]);
        this->skin = util::strToUShort(container[3]);
        this->gear = util::strToUShort(container[4]);
    }
}

std::ostream& operator<<(std::ostream& os, const PlayerState state) {
    os << state.serialize();
    return os;
}
