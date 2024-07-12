/*
 * Copyright (c) 2017 Jadavpur University, India
 *
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
 *
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/epc-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,                LTE+EPC를 위한 샘플 시뮬레이션 스크립트입니다. 여러 개의 eNB를 인스턴스화하고,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.     각 eNB 당 한 개의 UE를 연결하며, 각 UE에 대해 원격 호스트와의 플로우를 시작합니다.
 */

NS_LOG_COMPONENT_DEFINE("EpcFirstExampleForIpv6");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();                               // LTE 헬퍼 및 EPC 헬퍼 생성
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();                                            // EPC에서 P-G/W 노드 가져오기

    // Create a single RemoteHost                                                       원격 호스트 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                              인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    NodeContainer ueNodes;                                                              // UE 노드 및 eNB 노드 생성
    NodeContainer enbNodes;
    enbNodes.Create(2);
    ueNodes.Create(2);

    // Install Mobility Model                                                           모빌리티 모델 설정
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < 2; i++)
    {
        positionAlloc->Add(Vector(60.0 * i, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // Install the IP stack on the UEs                                                  UEs에 IP 스택 설치
    internet.Install(ueNodes);

    // Install LTE Devices to the nodes                                                 LTE 장치를 노드에 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    Ipv6InterfaceContainer ueIpIface;

    // Assign IP address to UEs                                                         IPv6 주소를 UE에 할당
    ueIpIface = epcHelper->AssignUeIpv6Address(NetDeviceContainer(ueLteDevs));

    Ipv6StaticRoutingHelper ipv6RoutingHelper;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UEs                                          각 UE에 대해 기본 게이트웨이 설정
        Ptr<Ipv6StaticRouting> ueStaticRouting =
            ipv6RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv6>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress6(), 1);
    }

    // Attach one UE per eNodeB                                                         각 UE를 해당 eNB에 연결
    for (uint16_t i = 0; i < 2; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated                        부작용: 기본 EPS 베어러가 활성화된다.
    }

    Ipv6AddressHelper ipv6h;                                                            // 인터넷 라우팅 설정
    ipv6h.SetBase(Ipv6Address("6001:db80::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign(internetDevices);

    internetIpIfaces.SetForwarding(0, true);
    internetIpIfaces.SetDefaultRouteInAllNodes(0);

    Ptr<Ipv6StaticRouting> remoteHostStaticRouting =                                    // 원격 호스트에 대한 정적 라우팅 추가
        ipv6RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv6>());
    remoteHostStaticRouting
        ->AddNetworkRouteTo("7777:f00d::", Ipv6Prefix(64), internetIpIfaces.GetAddress(0, 1), 1, 0);

    // interface 0 is localhost, 1 is the p2p device                                    원격 호스트 주소 설정 0은 로컬, 1은 p2p 장치
    Ipv6Address remoteHostAddr = internetIpIfaces.GetAddress(1, 1);

    // Start applications on UEs and remote host                                        UE 및 원격 호스트에 애플리케이션 설치 및 시작
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(remoteHost);

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    UdpEchoClientHelper echoClient1(remoteHostAddr, 9);
    UdpEchoClientHelper echoClient2(remoteHostAddr, 9);

    echoClient1.SetAttribute("MaxPackets", UintegerValue(1000));
    echoClient1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient1.SetAttribute("PacketSize", UintegerValue(1024));

    echoClient2.SetAttribute("MaxPackets", UintegerValue(1000));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps1 = echoClient1.Install(ueNodes.Get(0));
    ApplicationContainer clientApps2 = echoClient2.Install(ueNodes.Get(1));

    clientApps1.Start(Seconds(1.0));
    clientApps1.Stop(Seconds(14.0));

    clientApps2.Start(Seconds(1.5));
    clientApps2.Stop(Seconds(14.5));

    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);

    // PcapHelperForIpv6::EnablePcapIpv6 (std::string prefix, uint32_t nodeid, uint32_t interface,
    // bool explicitFilename)

    internet.EnablePcapIpv6("LenaIpv6-Ue-Rh-Ue0.pcap", ueNodes.Get(0)->GetId(), 1, true);
    internet.EnablePcapIpv6("LenaIpv6-Ue-Rh-Ue1.pcap", ueNodes.Get(1)->GetId(), 1, true);
    internet.EnablePcapIpv6("LenaIpv6-Ue-Rh-RH.pcap", remoteHostContainer.Get(0)->GetId(), 1, true);
    internet.EnablePcapIpv6("LenaIpv6-Ue-Rh-Pgw-Iface1.pcap", pgw->GetId(), 1, true);
    internet.EnablePcapIpv6("LenaIpv6-Ue-Rh-Pgw-Iface2.pcap", pgw->GetId(), 2, true);

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
