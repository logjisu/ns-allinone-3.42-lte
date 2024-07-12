/*
 * Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/applications-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC with different backhauls.                                   LTE+EPC 시뮬레이션 예제 코드입니다. 다양한 백홀을 사용하여 비교합니다.
 *
 * The purpose of this example is to compare:                                                       이 예제의 목적은 다음과 같습니다.
 *
 *  (1) how the simulation user can use a pre-existing EpcHelper that builds                        (1) 사전 정의된 EPC헬퍼(Ex: PointToPointEpcHelper)를 사용하여 백홀 네트워크
 *      a predefined backhaul network (e.g. the PointToPointEpcHelper) and                          를 구성하는 방
 *
 *  (2) how the simulation user can build its custom backhaul network in                            (2) 시뮬레이션 프로그램에서 사용자 정의 백홀 네트워크를 직접 구성하는 방법
 *      the simulation program (i.e. the point-to-point links are created
 *      in the simulation program instead of the pre-existing PointToPointEpcHelper)
 *
 * The pre-existing PointToPointEpcHelper is used with option --useHelper=1 and                     --useHelper=1 옵션으로 사전 정의된 PointToPointEpcHelper를 사용하거나.
 * the custom backhaul is built with option --useHelper=0                                           --useHelper=0 옵션으로 사용자 정의 백홀 네트워크를 구성할 수 있습니다.
 */

NS_LOG_COMPONENT_DEFINE("LenaSimpleEpcBackhaul");

int
main(int argc, char* argv[])
{
    uint16_t numNodePairs = 2;                                                                      // eNB + UE 쌍의 수
    Time simTime = MilliSeconds(1900);                                                              // 시뮬레이션 총 시간
    double distance = 60.0;                                                                         // eNB 사이의 거리 (미터)
    Time interPacketInterval = MilliSeconds(100);                                                   // 패킷 간 간격
    bool disableDl = false;                                                                         // 다운링크 데이터 흐름 비활성화 여부
    bool disableUl = false;                                                                         // 업링크 데이터 흐름 비활성화 여부
    bool useHelper = false;                                                                         // Helper 사용 여부

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);                     // eNodeB + UE pairs 수
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);                           // 시뮬레이션 총 시간
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);                                // eNB 사이 거리 [m]
    cmd.AddValue("interPacketInterval", "Inter packet interval", interPacketInterval);              // 패킷 간 간격
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);                            // 다운링크 데이터 흐름 비활성화 여부
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);                              // 업링크 데이터 흐름 비활성화 여부
    cmd.AddValue("useHelper",
                 "Build the backhaul network using the helper or "                                  // Helper를 사용하여 백홀 네트워크를 구성할지 여부
                 "it is built in the example",                                                      // (0: 사용자 정의 백홀, 1: 사전 정의된 Helper 사용)
                 useHelper);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                         명령행 인자를 다시 파싱하여 기본값을 재정의할 수 있도록 함
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<EpcHelper> epcHelper;
    if (!useHelper)
    {
        epcHelper = CreateObject<NoBackhaulEpcHelper>();                                            // 사용자 정의 백홀 Helper
    }
    else
    {
        epcHelper = CreateObject<PointToPointEpcHelper>();                                          // 사전 정의된 PointToPointEpcHelper
    }
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost                                                                   RemoteHost 하나 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                                          인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);                                    // RemoteHost의 주소

    Ipv4StaticRoutingHelper ipv4RoutingHelper;                                                      // RemoteHost에 정적 경로 추가
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;                                                                          // UE 노드 생성
    NodeContainer enbNodes;                                                                         // eNB 노드 생성
    enbNodes.Create(numNodePairs);
    ueNodes.Create(numNodePairs);

    // Install Mobility Model for eNBs and UEs                                                      eNB와 UE의 이동성 모델 설정
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < numNodePairs; i++)
    {
        positionAlloc->Add(Vector(distance * i, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // SGW node                                                                                     S-G/W 노드
    Ptr<Node> sgw = epcHelper->GetSgwNode();

    // Install Mobility Model for SGW                                                               S-G/W에 대한 이동성 모델 설치
    Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator>();
    positionAlloc2->Add(Vector(0.0, 50.0, 0.0));
    MobilityHelper mobility2;
    mobility2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility2.SetPositionAllocator(positionAlloc2);
    mobility2.Install(sgw);

    // Install LTE Devices to the nodes                                                             노드 LTE 장치 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    if (!useHelper)
    {
        Ipv4AddressHelper s1uIpv4AddressHelper;

        // Create networks of the S1 interfaces                                                     S1 인터페이스 네트워크 생성
        s1uIpv4AddressHelper.SetBase("10.0.0.0", "255.255.255.252");

        for (uint16_t i = 0; i < numNodePairs; ++i)
        {
            Ptr<Node> enb = enbNodes.Get(i);
            std::vector<uint16_t> cellIds(1, i + 1);

            // Create a point to point link between the eNB and the SGW with                        eNB와 S-G/W 사이의 점대점 링크 생성
            // the corresponding new NetDevices on each side
            PointToPointHelper p2ph;
            DataRate s1uLinkDataRate = DataRate("10Gb/s");
            uint16_t s1uLinkMtu = 2000;
            Time s1uLinkDelay = Time(0);
            p2ph.SetDeviceAttribute("DataRate", DataRateValue(s1uLinkDataRate));
            p2ph.SetDeviceAttribute("Mtu", UintegerValue(s1uLinkMtu));
            p2ph.SetChannelAttribute("Delay", TimeValue(s1uLinkDelay));
            NetDeviceContainer sgwEnbDevices = p2ph.Install(sgw, enb);

            Ipv4InterfaceContainer sgwEnbIpIfaces = s1uIpv4AddressHelper.Assign(sgwEnbDevices);
            s1uIpv4AddressHelper.NewNetwork();

            Ipv4Address sgwS1uAddress = sgwEnbIpIfaces.GetAddress(0);
            Ipv4Address enbS1uAddress = sgwEnbIpIfaces.GetAddress(1);

            // Create S1 interface between the SGW and the eNB                                      S-G/W와 eNB 사이의 S1 인터페이스 생성
            epcHelper->AddS1Interface(enb, enbS1uAddress, sgwS1uAddress, cellIds);
        }
    }

    // Install the IP stack on the UEs                                                              UE에 IP 스택 설치
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications                                           UE에 IP 주소 할당 및 애플리케이션 설치
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE                                                       UE의 기본 게이트웨이 설정
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Attach one UE per eNodeB                                                                     각 eNB에 하나의 UE를 연결
    for (uint16_t i = 0; i < numNodePairs; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated                                    부작용: 기본 EPS 베어러가 활성화 됨
    }

    // Install and start applications on UEs and remote host                                        UE와 원격 호스트에서 애플리케이션 설치 및 시작
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        if (!disableDl)
        {
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

            UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
            dlClient.SetAttribute("Interval", TimeValue(interPacketInterval));
            dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));
            clientApps.Add(dlClient.Install(remoteHost));
        }

        if (!disableUl)
        {
            ++ulPort;
            PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

            UdpClientHelper ulClient(remoteHostAddr, ulPort);
            ulClient.SetAttribute("Interval", TimeValue(interPacketInterval));
            ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));
            clientApps.Add(ulClient.Install(ueNodes.Get(u)));
        }
    }

    serverApps.Start(MilliSeconds(500));
    clientApps.Start(MilliSeconds(500));
    lteHelper->EnableTraces();
    // Uncomment to enable PCAP tracing
    // p2ph.EnablePcapAll("lena-simple-epc-backhaul");

    Simulator::Stop(simTime);
    Simulator::Run();

    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
    return 0;
}
