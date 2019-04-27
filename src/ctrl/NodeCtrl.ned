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
simple NodeCtrl
{
    parameters:
        @display("i=block/control");
        @signal[QdLength](type="double");
        // cycle to collect events from clients
        double roundCycle @unit(s);
        //double applyCycle @unit(s);
//        volatile double offset @unit(s);
        double cycleStart @unit(s);
        double GCCycle @unit(s);
	gates:
	    inout uplink;
	    inout udplink;
	    inout tcplink;        
}
