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

package ctrl;

//
// TODO auto-generated module
//
simple ClientCtrl
{
    parameters:
//        @class(Client);
        @display("i=block/broadcast");
        @signal[latency](type="double");
        //  Minimal time unit can only be second
        double initJoinTime @unit(s);
        double handshakeTime @unit(s);
        double loadCycle @unit(s);
        double discoveryCycle @unit(s);
        bool connect = default(true);
        bool deviate;
    gates:
        inout uplink;
        inout udplink;
        inout tcplink;
}
