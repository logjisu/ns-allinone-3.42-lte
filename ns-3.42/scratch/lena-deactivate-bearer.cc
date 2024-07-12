/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Gaurav Sathe <gaurav.sathe@tcs.com>
 */

#include "ns3/applications-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/epc-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates one eNodeB,                            LTE+EPC 샘플 시뮬레이션 스크립트입니다. 이 스크립트는 하나의 eNodeB를 
 * attaches three UE to eNodeB starts a flow for each UE to  and from a remote host.            인스턴스화하고,세 게의 UE를 eNodeB에 연결하여 각각의 UE에 대해 원격 호스트와의
 * It also instantiates one dedicated bearer per UE                                             흐름을 시작합니다. 또한 각 UE에 대해 하나의 전용 베어러를 인스턴스화합니다.
 */
NS_LOG_COMPONENT_DEFINE("BearerDeactivateExample");

int
main(int argc, char* argv[])
{
    uint16_t numberOfNodes = 1;                                                                 // 노드(eNB) 수
    uint16_t numberOfUeNodes = 3;                                                               // UE 노드 수
    double simTime = 1.1;                                                                       // 시뮬레이션 시간
    double distance = 60.0;                                                                     // eNB 간 거리
    double interPacketInterval = 100;                                                           // 패킷 간 거리

    // Command line arguments                                                                   명령줄 인자
    CommandLine cmd(__FILE__);
    cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                     기본값을 명령줄에서 재설정할 수 있도록 다시 파싱
    cmd.Parse(argc, argv);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Enable Logging                                                                           로깅 활성화
    auto logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

    LogComponentEnable("BearerDeactivateExample", LOG_LEVEL_ALL);
    LogComponentEnable("LteHelper", logLevel);
    LogComponentEnable("EpcHelper", logLevel);
    LogComponentEnable("EpcEnbApplication", logLevel);
    LogComponentEnable("EpcMmeApplication", logLevel);
    LogComponentEnable("EpcPgwApplication", logLevel);
    LogComponentEnable("EpcSgwApplication", logLevel);
    LogComponentEnable("LteEnbRrc", logLevel);

    // Create a single RemoteHost                                                               단일 원격 호스트 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                                      인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device                                            인터페이스 0은 localhost, 1은 p2p 디바이스
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfNodes);
    ueNodes.Create(numberOfUeNodes);

    // Install Mobility Model                                                                   이동성 모델 설치
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < numberOfNodes; i++)
    {
        positionAlloc->Add(Vector(distance * i, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // Install LTE Devices to the nodes                                                         노드에 LTE 디바이스 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs                                                          UE에 IP 스택 설치
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications                                       UE에 IP 주소 할당 및 애플리케이션 설치
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE                                                   UE의 기본 게이트웨이 설정
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Attach a UE to a eNB                                                                     UE를 eNB에 연결
    lteHelper->Attach(ueLteDevs, enbLteDevs.Get(0));

    // Activate an EPS bearer on all UEs                                                        모든 UE에 EPS 베어러 활성화

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<NetDevice> ueDevice = ueLteDevs.Get(u);
        GbrQosInformation qos;
        qos.gbrDl = 132; // bit/s, considering IP, UDP, RLC, PDCP header size                   bit/s, IP, UDP, RLC, PDCP 헤더 크기 고려
        qos.gbrUl = 132;
        qos.mbrDl = qos.gbrDl;
        qos.mbrUl = qos.gbrUl;

        EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
        EpsBearer bearer(q, qos);
        bearer.arp.priorityLevel = 15 - (u + 1);
        bearer.arp.preemptionCapability = true;
        bearer.arp.preemptionVulnerability = true;
        lteHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, EpcTft::Default());
    }

    // Install and start applications on UEs and remote host                                    UE 및 원격 호스트에 애플리케이션 설치 및 시작
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    uint16_t otherPort = 3000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        ++ulPort;
        ++otherPort;
        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                          InetSocketAddress(Ipv4Address::GetAny(), otherPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
        serverApps.Add(packetSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        UdpClientHelper client(ueIpIface.GetAddress(u), otherPort);
        client.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        client.SetAttribute("MaxPackets", UintegerValue(1000000));

        clientApps.Add(dlClient.Install(remoteHost));
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));
        if (u + 1 < ueNodes.GetN())
        {
            clientApps.Add(client.Install(ueNodes.Get(u + 1)));
        }
        else
        {
            clientApps.Add(client.Install(ueNodes.Get(0)));
        }
    }

    serverApps.Start(Seconds(0.030));
    clientApps.Start(Seconds(0.030));

    double statsStartTime = 0.04; // need to allow for RRC connection establishment + SRS               RRC 연결 설정 및 SRS를 허용해야 함
    double statsDuration = 1.0;

    lteHelper->EnableRlcTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("StartTime", TimeValue(Seconds(statsStartTime)));
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(statsDuration)));

    // get ue device pointer for UE-ID 0 IMSI 1 and enb device pointer                                  UE-ID 0 IMSI 1 및 eNB 디바이스 포인터에 대한 UE 디바이스 포인터 가져오기
    Ptr<NetDevice> ueDevice = ueLteDevs.Get(0);
    Ptr<NetDevice> enbDevice = enbLteDevs.Get(0);

    /*
     *   Instantiate De-activation using Simulator::Schedule() method which will initiate bearer        Simulator::Schedule() 매서드를 사용하여 비활성화를 인스턴스화하여
     * de-activation after deActivateTime Instantiate De-activation in sequence (Time const &time,      deActivateTime 이후에 베어러 비활성화를 시작합니다.
     * MEM mem_ptr, OBJ obj, T1 a1, T2 a2, T3 a3)                                                       순차적으로 비활성화 인스턴스화
     */
    Time deActivateTime(Seconds(1.5));
    Simulator::Schedule(deActivateTime,
                        &LteHelper::DeActivateDedicatedEpsBearer,
                        lteHelper,
                        ueDevice,
                        enbDevice,
                        2);

    // stop simulation after 3 seconds                                                                  3초 후 시뮬레이션 중지
    Simulator::Stop(Seconds(3.0));

    Simulator::Run();
    /*GtkConfigStore config;
    config.ConfigureAttributes();*/

    Simulator::Destroy();
    return 0;
}
