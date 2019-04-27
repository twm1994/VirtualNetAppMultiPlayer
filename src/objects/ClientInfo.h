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

#ifndef OBJECTS_CLIENTINFO_H_
#define OBJECTS_CLIENTINFO_H_

#include "IPvXAddress.h"

using namespace std;

class ClientInfo {
private:
    string name;
    unsigned long locX;
    unsigned long locY;
    IPvXAddress rendezvous;
public:
    ClientInfo();
    ClientInfo(string clienName, unsigned long x, unsigned long y);
    ClientInfo(string clienName, unsigned long x, unsigned long y, IPvXAddress rendezvous);
    void setRendezvous(IPvXAddress rendezvous);
    IPvXAddress getRendezvous();
    virtual ~ClientInfo();
    unsigned long getX();
    unsigned long getY();
    void setX(unsigned long x);
    void setY(unsigned long y);
    string getName() const;
    string str() const;

    friend ostream& operator<<(ostream& os, const ClientInfo& info) {
        return os << info.str();
    }

    friend bool operator==(const ClientInfo& info1, const ClientInfo& info2) {
        return info1.getName() == info2.getName();

    }

    friend bool operator!=(const ClientInfo& info1, const ClientInfo& info2) {
        return info1.getName() != info2.getName();

    }

    friend bool operator<(const ClientInfo& info1, const ClientInfo& info2) {
        return info1.getName() < info2.getName();

    }
};

#endif /* OBJECTS_CLIENTINFO_H_ */
