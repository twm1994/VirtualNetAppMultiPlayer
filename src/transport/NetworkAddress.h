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

#ifndef TRANSPORT_NETWORKADDRESS_H_
#define TRANSPORT_NETWORKADDRESS_H_

#include <cstdint>

class NetworkAddress {
private:
    static uint32_t nextFreeAddress;
public:
    NetworkAddress();
    virtual ~NetworkAddress();

    static uint32_t freeAddress() {
        return nextFreeAddress++;
    }
};

#endif /* TRANSPORT_NETWORKADDRESS_H_ */
