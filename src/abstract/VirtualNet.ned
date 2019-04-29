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

package abstract;

import abstract.*;
import global.*;
import hosts.*;

network VirtualNet
{
    parameters:
        @display("bgb=1080,720;");
        @statistic[InteractionLatency](source=latency; title="Interaction Latency"; record=stats);
        @statistic[DeliveryQueueLength](source=QdLength; title="Delivery queue length"; record=stats);
        int client_num = default(1);
        double pktLossRate;
        double Copt @unit(mps);
        double BW @unit(bps);
        double MTU @unit(b);
        double jitter_mu;
        double jitter_sigma;
        string churnGeneratorModule;
        double Dmin @unit(s);
        volatile double jitter @unit(s);
    submodules:
        globalObserver: GlobalObserver {
            @display("p=100,50;t=Observer");
        }
        underlayConfigurator: UnderlayConfigurator {
            @display("p=100,200;t=Network Configurator");
        }
        coordinator: Coordinator {
            @display("p=100,350;t=Virutal Coordinator");
        }
        ContentStorage: ChordOverlay {
            @display("p=400,50;t=Peer-to-peer Cloud Service");
        }
        //        churnGenerator: UniformChurnGenerator {
        //            @display("p=100,500;t=Churn Generator");
        //        }
        churnGenerator: WeibullChurnGenerator {
            @display("p=100,500;t=Churn Generator");
        }
    //        churnGenerator: ManualChurnGenerator {
    //            @display("p=100,500;t=Churn Generator");
    //        }
    connections allowunconnected:
}
