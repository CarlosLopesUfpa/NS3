/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"

#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/lte-module.h"

// Bibliotecas para o FlowMonitor
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-module.h"
#include <ns3/flow-monitor-helper.h>

using namespace ns3;

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);
void JitterMonitor(FlowMonitorHelper *fmHelper, Ptr<FlowMonitor> flowMon, Gnuplot2dDataset Dataset2);
void DelayMonitor(FlowMonitorHelper *fmHelper, Ptr<FlowMonitor> flowMon, Gnuplot2dDataset Dataset3);

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  bool Random = false; 
  double speed = 5;
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("EvalvidClientHelper", LOG_LEVEL_INFO);
  LogComponentEnable ("EvalvidServerHelper", LOG_LEVEL_INFO);

  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("2s"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));


  NodeContainer server;
  server.Create (1);

  NodeContainer nodes;
  nodes.Create (3);

  NodeContainer all;
  all.Add(server);
  all.Add(nodes);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi; 
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;

  NetDeviceContainer devices;

  Ssid ssid;

  ssid = Ssid ("network-A");
  phy.Set ("ChannelNumber", UintegerValue (36));
  mac.SetType ("ns3::AdhocWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));
  devices = wifi.Install (phy, mac, nodes);

  
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (50000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));


  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  // Uncomment to enable logging
  //  lteHelper->EnableLogComponents ();

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbNode;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbNode = lteHelper->InstallEnbDevice (server);

  // Attach a UE to a eNB
  lteHelper->Attach (nodes.Get(0), server);


NetDeviceContainer ueNodes = csma.Install (nodes);

InternetStackHelper internet;
internet.Install (nodes);


  MobilityHelper mobility; 
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0), //onde inicia no eixo X
                                 "MinY", DoubleValue (0.0), //onde inicia no eixo Y
                                 "DeltaX", DoubleValue (10.0), // Distância entre nós
                                 "DeltaY", DoubleValue (10.0), // Distância entre nós
                                 "GridWidth", UintegerValue (12), // Quantidade de colunas em uma linha
                                 "LayoutType", StringValue ("RowFirst")); // Definindo posições em linha
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

if (Random) {
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                           "Mode", StringValue ("Time"),
                           "Time", StringValue ("2s"),
                           "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                           "Bounds", StringValue ("0|200|0|200"));
}
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (server);
  server.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0, 50, 10));
  server.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  InternetStackHelper stack;
  stack.Install (all);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer ueNodesD = address.Assign (devices);

  address.SetBase ("10.1.2.0", "255.255.255.0");

  Ipv4InterfaceContainer enbNodeD = address.Assign (enbNode);


  NS_LOG_INFO ("Create Applications.");
  
  EvalvidServerHelper enbNodeD (80);
  enbNodeD.SetAttribute ("SenderTraceFilename", StringValue("st_highway_cif.st"));
  enbNodeD.SetAttribute ("SenderDumpFilename", StringValue("sd_a01_lte"));
  ApplicationContainer apps = enbNodeD.Install(enbNodeD);
  apps.Start (Seconds (9.0));
  apps.Stop (Seconds (101.0));
  
  EvalvidClientHelper client (internetIpIfaces.GetAddress (1),port);
  client.SetAttribute ("ReceiverDumpFilename", StringValue("rd_a01_lte"));
  apps = client.Install (ueNodesD);
  apps.Start (Seconds (10.0));
  apps.Stop (Seconds (90.0));
  

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // pointToPoint.EnablePcapAll ("tres", false);



//-----------------FlowMonitor-THROUGHPUT----------------

    std::string fileNameWithNoExtension = "FlowVSThroughput_";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs Throughput";
    std::string dataTitle               = "Throughput";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Flow", "Throughput");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  // call the flow monitor function
  ThroughputMonitor(&fmHelper, allMon, dataset); 

//-----------------FlowMonitor-JITTER--------------------

    std::string fileNameWithNoExtension2 = "FlowVSJitter_";
    std::string graphicsFileName2      = fileNameWithNoExtension2 + ".png";
    std::string plotFileName2        = fileNameWithNoExtension2 + ".plt";
    std::string plotTitle2           = "Flow vs Jitter";
    std::string dataTitle2           = "Jitter";

    Gnuplot gnuplot2 (graphicsFileName2);
    gnuplot2.SetTitle(plotTitle2);

  gnuplot2.SetTerminal("png");

  gnuplot2.SetLegend("Flow", "Jitter");

  Gnuplot2dDataset dataset2;
  dataset2.SetTitle(dataTitle2);
  dataset2.SetStyle(Gnuplot2dDataset::LINES_POINTS);

  //FlowMonitorHelper fmHelper;
  //Ptr<FlowMonitor> allMon = fmHelper.InstallAll();

  JitterMonitor(&fmHelper, allMon, dataset2);

//-----------------FlowMonitor-DELAY---------------------

  std::string fileNameWithNoExtension3 = "FlowVSDelay_";
  std::string graphicsFileName3      = fileNameWithNoExtension3 + ".png";
  std::string plotFileName3        = fileNameWithNoExtension3 + ".plt";
  std::string plotTitle3       = "Flow vs Delay";
  std::string dataTitle3       = "Delay";

      Gnuplot gnuplot3 (graphicsFileName3);
      gnuplot3.SetTitle(plotTitle3);

      gnuplot3.SetTerminal("png");

      gnuplot3.SetLegend("Flow", "Delay");

      Gnuplot2dDataset dataset3;
      dataset3.SetTitle(dataTitle3);
      dataset3.SetStyle(Gnuplot2dDataset::LINES_POINTS);

      DelayMonitor(&fmHelper, allMon, dataset3);


//-------------------------Metodo-Animation-------------------------

      AnimationInterface anim ("tres.xml"); // Mandatory
      for (uint32_t i = 0; i < nodes.GetN (); ++i)
      {
        anim.UpdateNodeDescription (nodes.Get (i), "NODE"); // Optional
        anim.UpdateNodeColor (nodes.Get (i), 255, 0, 0); // Coloração
      }
      for (uint32_t i = 0; i < server.GetN (); ++i)
      {
        anim.UpdateNodeDescription (server.Get (i), "SERVER"); // Optional
        anim.UpdateNodeColor (server.Get (i), 0, 255, 0); // Coloração
      }
      anim.EnablePacketMetadata (); // Optional

  Simulator::Stop (Seconds (15.0));
  Simulator::Run ();
  
//Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();

//---ContJITTER---

  gnuplot2.AddDataset(dataset2);;
  std::ofstream plotFile2 (plotFileName2.c_str());
  gnuplot2.GenerateOutput(plotFile2);
  plotFile2.close();

  //---ContDelay---

  gnuplot3.AddDataset(dataset3);;
  std::ofstream plotFile3 (plotFileName3.c_str());
  gnuplot3.GenerateOutput(plotFile3);
  plotFile3.close();


  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  return 0;
}
//-------------------------Metodo-VAZÃO---------------------------

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
  {
        double localThrou=0;
    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
                      if(stats->first == 2){//IFFFFFFFFFFFFFFFFFFFFFFF
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
      std::cout<<"Flow ID     : " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
      std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
      std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
            std::cout<<"Duration    : "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
      std::cout<<"Last Received Packet  : "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
      std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
            localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
      // updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
      std::cout<<"---------------------------------------------------------------------------"<<std::endl;
  }//IFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF  
}
      Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
  flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      }

  }

//-------------------------Metodo-JINTTER-------------------------

double atraso1=0;
void JitterMonitor(FlowMonitorHelper *fmHelper, Ptr<FlowMonitor> flowMon, Gnuplot2dDataset Dataset2)
{
       double localJitter=0;
       double atraso2 =0;

       std::map<FlowId, FlowMonitor::FlowStats> flowStats2 = flowMon->GetFlowStats();
       Ptr<Ipv4FlowClassifier> classing2 = DynamicCast<Ipv4FlowClassifier> (fmHelper->GetClassifier());
       for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats2 = flowStats2.begin(); stats2 != flowStats2.end(); ++stats2)
       {
               if(stats2->first == 2){//IFFFFFFFFFFF
            Ipv4FlowClassifier::FiveTuple fiveTuple2 = classing2->FindFlow (stats2->first);
    std::cout<<"Flow ID : "<< stats2->first <<";"<< fiveTuple2.sourceAddress <<"------>" <<fiveTuple2.destinationAddress<<std::endl;
    std::cout<<"Tx Packets = " << stats2->second.txPackets<<std::endl;
    std::cout<<"Rx Packets = " << stats2->second.rxPackets<<std::endl;
    std::cout<<"Duration  : "<<(stats2->second.timeLastRxPacket.GetSeconds()-stats2->second.timeFirstTxPacket.GetSeconds())<<std::endl;
    std::cout<<"Atraso: "<<stats2->second.timeLastRxPacket.GetSeconds()-stats2->second.timeLastTxPacket.GetSeconds() <<"s"<<std::endl;
atraso2 = stats2->second.timeLastRxPacket.GetSeconds()-stats2->second.timeLastTxPacket.GetSeconds();
//atraso1 = stats2->second.timeFirstRxPacket.GetSeconds()-stats2->second.timeFirstTxPacket.GetSeconds();
    std::cout<<"Jitter: "<< atraso2-atraso1 <<std::endl;
    localJitter= atraso2-atraso1;//Jitter
    Dataset2.Add((double)Simulator::Now().GetSeconds(), (double) localJitter);
    std::cout<<"---------------------------------------------------------------------------"<<std::endl;
               }//IFFFFFFFFFF

       atraso1 = atraso2;
       }

       Simulator::Schedule(Seconds(1),&JitterMonitor, fmHelper, flowMon, Dataset2);
       {
         flowMon->SerializeToXmlFile("JitterMonitor.xml", true, true);
       }
}

//-------------------------Metodo-DELAY---------------------------

void  DelayMonitor(FlowMonitorHelper *fmHelper, Ptr<FlowMonitor> flowMon, Gnuplot2dDataset Dataset3)
{
  double localDelay=0;

  std::map<FlowId, FlowMonitor::FlowStats> flowStats3 = flowMon->GetFlowStats();
  Ptr<Ipv4FlowClassifier> classing3 = DynamicCast<Ipv4FlowClassifier> (fmHelper->GetClassifier());
  for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats3 = flowStats3.begin(); stats3 != flowStats3.end(); ++stats3)
  {
               if(stats3->first == 2){//IFFFFFFFFFFF
          Ipv4FlowClassifier::FiveTuple fiveTuple3 = classing3->FindFlow (stats3->first);
          std::cout<<"Flow ID : "<< stats3->first <<";"<< fiveTuple3.sourceAddress <<"------>" <<fiveTuple3.destinationAddress<<std::endl;
          localDelay = stats3->second.timeLastRxPacket.GetSeconds()-stats3->second.timeLastTxPacket.GetSeconds();
        Dataset3.Add((double)Simulator::Now().GetSeconds(), (double) localDelay);
        std::cout<<"---------------------------------------------------------------------------"<<std::endl;
    }//IFFFFFFFFF
  }
  Simulator::Schedule(Seconds(1),&DelayMonitor, fmHelper, flowMon, Dataset3);
  {
     flowMon->SerializeToXmlFile("DelayMonitor.xml", true, true);
  }
}