/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Authors:
 *   Jaume Nin <jaume.nin@cttc.cat>
 *   Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeBs,                           LTE+EPC를 위한 샘플 시뮬레이션 스크립트입니다. 여러 eNB를 생성하고,
 * attaches one UE per eNodeB starts a flow for each UE to and from a remote host.                  각 eNB 당 하나의 UE를 연결하며, 각 UE에 대해 원격 호스트와의 데이터 흐름을
 * It also starts another flow between each UE pair.                                                시작합니다. 또한 각 UE 쌍 사이의 추가 데이터 흐름도 시작합니다.
 */

NS_LOG_COMPONENT_DEFINE("LenaSimpleEpc");

int
main(int argc, char* argv[])
{
    uint16_t numNodePairs = 2;                                                                      // eNB + UE 쌍의 수
    Time simTime = MilliSeconds(1100);                                                              // 시뮬레이션 총 시간
    double distance = 60.0;                                                                         // eNB 간 거리 (미터)
    Time interPacketInterval = MilliSeconds(100);                                                   // 패킷 간 간격
    bool useCa = false;                                                                             // 캐리어 집합 사용 여부
    bool disableDl = false;                                                                         // 다운링크 데이터 흐름 비활성화 여부
    bool disableUl = false;                                                                         // 업링크 데이터 흐름 비활성화 여부
    bool disablePl = false;                                                                         // PEER 간 데이터 흐름 비활성화 여부

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);                     // eNB + UE 쌍의 수
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);                           // 시뮬레이션 총 시간
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);                                // eNB 간 거리 (미터)
    cmd.AddValue("interPacketInterval", "Inter packet interval", interPacketInterval);              // 패킷 간 간격
    cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);                            // 캐리어 집합 사용 여부
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);                            // 다운링크 데이터 흐름 비활성화 여부
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);                              // 업링크 데이터 흐름 비활성화 여부
    cmd.AddValue("disablePl", "Disable data flows between peer UEs", disablePl);                    // PEER 간 데이터 흐름 비활성화 여부
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                         명령행 인자를 사용하여 기본값을 재정의할 수 있도록 다시 파싱
    cmd.Parse(argc, argv);

    if (useCa)                                                                                      // 캐리어 집합 사용 설정
    {
        Config::SetDefault("ns3::LteHelper::UseCa", BooleanValue(useCa));
        Config::SetDefault("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue(2));
        Config::SetDefault("ns3::LteHelper::EnbComponentCarrierManager",
                           StringValue("ns3::RrComponentCarrierManager"));
    }

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost                                                                   단일 원격 호스트 생성
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
    // interface 0 is localhost, 1 is the p2p device                                                인터페이스 0은 로컬호스트, 1은 p2p 장치
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;                                                      // 원격 호스트에 정적 경로 추가
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numNodePairs);
    ueNodes.Create(numNodePairs);

    // Install Mobility Model                                                                       // 이동성 모델 설치
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

    // Install LTE Devices to the nodes                                                             LTE 장치를 노드에 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

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

    // Attach one UE per eNodeB                                                                     각 eNB에 하나의 UE 연결
    for (uint16_t i = 0; i < numNodePairs; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated                                    부작용: 기본 EPS 베어러가 활성화됨
    }

    // Install and start applications on UEs and remote host                                        UE 및 원격 호스트에 애플리케이션 설치 및 시작
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    uint16_t otherPort = 3000;
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

        if (!disablePl && numNodePairs > 1)
        {
            ++otherPort;
            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                              InetSocketAddress(Ipv4Address::GetAny(), otherPort));
            serverApps.Add(packetSinkHelper.Install(ueNodes.Get(u)));

            UdpClientHelper client(ueIpIface.GetAddress(u), otherPort);
            client.SetAttribute("Interval", TimeValue(interPacketInterval));
            client.SetAttribute("MaxPackets", UintegerValue(1000000));
            clientApps.Add(client.Install(ueNodes.Get((u + 1) % numNodePairs)));
        }
    }

    serverApps.Start(MilliSeconds(500));
    clientApps.Start(MilliSeconds(500));
    lteHelper->EnableTraces();
    // Uncomment to enable PCAP tracing                                                             PACP 추적을 활성화하려면 주석 해제
    // p2ph.EnablePcapAll("lena-simple-epc");

    Simulator::Stop(simTime);
    Simulator::Run();

    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
    return 0;
}
