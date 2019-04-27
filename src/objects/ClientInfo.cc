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

#include "ClientInfo.h"

ClientInfo::ClientInfo() {
    // TODO Auto-generated constructor stub

}

ClientInfo::ClientInfo(string clientName, unsigned long x, unsigned long y) {
    this->name = clientName;
    this->locX = x;
    this->locY = y;
}

ClientInfo::ClientInfo(string clientName, unsigned long x, unsigned long y,
        IPvXAddress rendezvous) {
    this->name = clientName;
    this->locX = x;
    this->locY = y;
    this->rendezvous = rendezvous;
}

ClientInfo::~ClientInfo() {
    // TODO Auto-generated destructor stub
}

unsigned long ClientInfo::getX() {
    return locX;
}

unsigned long ClientInfo::getY() {
    return locY;
}

void ClientInfo::setX(unsigned long x){
    locX = x;
}

void ClientInfo::setY(unsigned long y){
    locY = y;
}

string ClientInfo::getName() const {
    return name;
}

void ClientInfo::setRendezvous(IPvXAddress rendezvous) {
    this->rendezvous = rendezvous;
}

IPvXAddress ClientInfo::getRendezvous(){
    return this->rendezvous;
}

string ClientInfo::str() const {
    stringstream ss;
    ss << name << " (x = " << locX << ", y = " << locY << ", "
            << rendezvous.get4().str() << ")";
    return ss.str();
}
