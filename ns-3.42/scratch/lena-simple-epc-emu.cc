/*
 * Copyright (c) 2011, 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
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

using namespace ns3;

/*
 * Simple simulation program using the emulated EPC.                                                Emulated EPC를 사용한 간단한 시뮬레이션 프로그램.
 * For the LTE radio part, it simulates a simple linear topology with                               LTE 라디오 부분에서는 고정된 수의 eNB가 일정한 거리 간격으로 배치되고,
 * a fixed number of eNBs spaced at equal distance, and a fixed number                              각 eNB마다 일정 수의 UE가 eNB와 동일한 위치에 위치합니다.
 * of UEs per each eNB, located at the same position of the eNB.                                    EPC에서는 EmuEpcHelper를 사용하여 실제 링크를 통해 S1-U 연결을 실현합
 * For the EPC, it uses EmuEpcHelper to realize the S1-U connection                                 니다.
 * via a real link.
 */

NS_LOG_COMPONENT_DEFINE("EpcFirstExample");

int
main(int argc, char* argv[])
{
    uint16_t nEnbs = 1;                                                                             // eNB 수
    uint16_t nUesPerEnb = 1;                                                                        // 각 eNB 당 UE 수
    double simTime = 10.1;                                                                          // 시뮬레이션 총 시간 (초)
    double distance = 1000.0;                                                                       // eNB 간 거리 (미터)
    double interPacketInterval = 1000;                                                              // 패킷 간 간격 (ms)

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("nEnbs", "Number of eNBs", nEnbs);                                                 // eNB 수
    cmd.AddValue("nUesPerEnb", "Number of UEs per eNB", nUesPerEnb);                                // 각 eNB 당 UE 수
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);                      // 시뮬레이션 총 시간
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);                                // eNB 간 거리
    cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);        // 패킷 간 간격
    cmd.Parse(argc, argv);

    // let's go in real time                                                                        실시간 시뮬레이션 사용 설정(실시간 시뮬레이션일 경우 권장됨)
    // NOTE: if you go in real time I strongly advise to use
    // --ns3::RealtimeSimulatorImpl::SynchronizationMode=HardLimit
    // I've seen that if BestEffort is used things can break
    // (even simple stuff such as ARP)
    // GlobalValue::Bind ("SimulatorImplementationType",
    //                 StringValue ("ns3::RealtimeSimulatorImpl"));

    // let's speed things up, we don't need these details for this scenario                         LTE 스펙트럼 물리 계층의 오류 모델 비활성화
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(false));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(false));

    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));                                       // IP 계층에서 체크섬 활성화

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                         명령행 인자를 사용하여 기본값을 재정의할 수 있도록 다시 파싱
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();                                           // LTE Helper와 EmuEPCHelper 생성
    Ptr<EmuEpcHelper> epcHelper = CreateObject<EmuEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);                                                             // LTE Helper에 EmuEPCHelper 설정
    epcHelper->Initialize();                                                                        // EmuEpcHelper 초기화

    Ptr<Node> pgw = epcHelper->GetPgwNode();                                                        // P-G/W 노드 가져오기

    // Create a single RemoteHost                                                                   // 원격 호스트 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                                          // 인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));                                   // 링크 지연 설정
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device                                                // 인터페이스 0은 로컬호스트, 1은 p2p 장치
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;                                                      // 원격 호스트에 정적 경로 추가
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;                                                                          // eNB 노드와 UE 노드 생성 및 이동성 모델 설정
    NodeContainer enbNodes;
    enbNodes.Create(nEnbs);
    ueNodes.Create(nEnbs * nUesPerEnb);

    // Install Mobility Model                                                                       // 이동성 모델 설치
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < nEnbs; i++)
    {
        positionAlloc->Add(Vector(distance * i, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // Install LTE Devices to the nodes                                                             // LTE 장치를 노드에 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs                                                              // UE에 IP 스택 설치
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications                                           // UE에 IP 주소 할당 및 애플리케이션 설치
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE                                                       // UE의 기본 게이트웨이 설정
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    lteHelper->Attach(ueLteDevs);                                                                   // 각 UE를 eNB에 연결
    // side effects: 1) use idle mode cell selection, 2) activate default EPS bearer                부작용: idle mode 셀 선택, 기본 EPS 베어러 활성화

    // randomize a bit start times to avoid simulation artifacts                                    시작 시간을 약간 랜덤화하여 시뮬레이션 오버플로우 방지
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(interPacketInterval / 1000.0));

    // Install and start applications on UEs and remote host                                        UE 및 원격 호스트에 애플리케이션 설치 및 시작
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        ++ulPort;
        ApplicationContainer clientApps;
        ApplicationContainer serverApps;

        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                            InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));                                 // DL 패킷 싱크 설치
        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));                                     // UL 패킷 싱크 설치

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        UdpClientHelper ulClient(remoteHostAddr, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        clientApps.Add(dlClient.Install(remoteHost));                                               // DL 클라이언트 애플리케이션 설치
        clientApps.Add(ulClient.Install(ueNodes.Get(u)));                                           // UL 클라이언트 애플리케이션 설치

        serverApps.Start(Seconds(startTimeSeconds->GetValue()));
        clientApps.Start(Seconds(startTimeSeconds->GetValue()));
    }

    Simulator::Stop(Seconds(simTime));                                                              // 시뮬레이션 종료 시간 설정
    Simulator::Run();                                                                               // 시뮬레이션 실행

    Simulator::Destroy();                                                                           // 시뮬레이션 종료 및 객체 정리
    return 0;
}
