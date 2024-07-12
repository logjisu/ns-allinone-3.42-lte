/*
 * Copyright (c) 2012-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaX2HandoverExample");

/**
 * UE Connection established notification.                                                              UE 연결 설정 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": connected to CellId " << cellid << " with RNTI " << rnti << std::endl;
}

/**
 * UE Start Handover notification.                                                                      UE 핸드오버 시작 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The actual Cell ID.                                                                    현재 셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 * \param targetCellId The target Cell ID.                                                              목표 셀 ID
 */
void
NotifyHandoverStartUe(std::string context,
                      uint64_t imsi,
                      uint16_t cellid,
                      uint16_t rnti,
                      uint16_t targetCellId)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi                  
              << ": previously connected to CellId " << cellid << " with RNTI " << rnti                 // 현재 셀 ID에 연결된 RNTI
              << ", doing handover to CellId " << targetCellId << std::endl;                            // 셀 ID로 핸드오버 시작
}

/**
 * UE Handover end successful notification.                                                             UE 핸드오버 오나료 성공 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": successful handover to CellId " << cellid << " with RNTI " << rnti << std::endl;
}

/**
 * eNB Connection established notification.                                                             eNB 연결 설정 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": successful connection of UE with IMSI " << imsi << " RNTI " << rnti                 // IMSI에 대한 UE 연결 성공, RNTI
              << std::endl;
}

/**
 * eNB Start Handover notification.                                                                     eNB 핸드오버 시작 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The actual Cell ID.                                                                    현재 셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 * \param targetCellId The target Cell ID.                                                              목표 셀 ID
 */
void
NotifyHandoverStartEnb(std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": start handover of UE with IMSI " << imsi << " RNTI " << rnti << " to CellId "       // IMSI에 대한 UE RNTI 핸드오버 시작, 목표 셀 ID로
              << targetCellId << std::endl;
}

/**
 * eNB Handover end successful notification.                                                            eNB 핸드오버 완료 성공 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyHandoverEndOkEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": completed handover of UE with IMSI " << imsi << " RNTI " << rnti << std::endl;      // IMSI에 대한 UE RNTI 핸드오버 완료
}

/**
 * Handover failure notification                                                                        핸드오버 실패 알림
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI of the connected terminal.                                                      연결된 단말의 IMSI
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyHandoverFailure(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << " IMSI " << imsi << " RNTI " << rnti << " handover failure" << std::endl;
}

/**
 * Sample simulation script for a X2-based handover.                                                    X2 기반 핸드오버의 샘플 시뮬레이션 스크립트
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB and                                  두 개의 eNB를 생성하고, 하나의 UE를 소스 eNB에 연결하고,
 * triggers a handover of the UE towards the 'target' eNB.                                              UE를 타겟 eNB로의 핸드오버를 트리거합니다.
 */
int
main(int argc, char* argv[])
{
    // LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

    // LogComponentEnable ("LteHelper", logLevel);
    // LogComponentEnable ("EpcHelper", logLevel);
    // LogComponentEnable ("EpcEnbApplication", logLevel);
    // LogComponentEnable ("EpcMmeApplication", logLevel);
    // LogComponentEnable ("EpcPgwApplication", logLevel);
    // LogComponentEnable ("EpcSgwApplication", logLevel);
    // LogComponentEnable ("EpcX2", logLevel);

    // LogComponentEnable ("LteEnbRrc", logLevel);
    // LogComponentEnable ("LteEnbNetDevice", logLevel);
    // LogComponentEnable ("LteUeRrc", logLevel);
    // LogComponentEnable ("LteUeNetDevice", logLevel);

    uint16_t numberOfUes = 1;                                                                           // UE 수
    uint16_t numberOfEnbs = 2;                                                                          // eNB 수
    uint16_t numBearersPerUe = 2;                                                                       // UE 당 베어러 수
    Time simTime = MilliSeconds(490);                                                                   // 시뮬레이션 시간
    double distance = 100.0;                                                                            // eNB 사이의 거리
    bool disableDl = false;                                                                             // 다운링크 데이터 흐름 비활성화 여부
    bool disableUl = false;                                                                             // 업링크 데이터 흐름 비활성화 여부

    // change some default attributes so that they are reasonable for                                   시나리오에 맞는 기본 속성 변경
    // this scenario, but do this before processing command line
    // arguments, so that the user is allowed to override these settings
    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(10)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(false));

    // Command line arguments                                                                           명령 줄 인수 처리
    CommandLine cmd(__FILE__);
    cmd.AddValue("numberOfUes", "Number of UEs", numberOfUes);                                          // UE 수
    cmd.AddValue("numberOfEnbs", "Number of eNodeBs", numberOfEnbs);                                    // eNB 수
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);                               // 시뮬레이션 총 시간
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);                                // 다운링크 데이터 흐름 비활성화
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);                                  // 업링크 데이터 흐름 비활성화
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm"); // disable automatic handover    자동 핸드오버 비활성화

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost                                                                       단일 RemoteHost 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                                              인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Routing of the Internet Host (towards the LTE network)                                           인터넷 호스트의 라우팅 (LTE 네트워크 방향)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    // interface 0 is localhost, 1 is the p2p device                                                    인터페이스 0은 로컬호스트, 1은 P2P 디바이스
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    // Install Mobility Model                                                                           이동성 모델 설치
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
        positionAlloc->Add(Vector(distance * 2 * i - distance, 0, 0));
    }
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        positionAlloc->Add(Vector(0, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);

    // Install LTE Devices in eNB and UEs                                                               eNB 및 UE에 LTE 장치 설치
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs                                                                  UE에 IP 스택 설치
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    // Attach all UEs to the first eNodeB                                                               모든 UE를 첫 번째 eNB에 연결
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));
    }

    NS_LOG_LOGIC("setting up applications");

    // Install and start applications on UEs and remote host                                            UE 및 원격 호스트에 애플리케이션 설치 및 시작
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    // randomize a bit start times to avoid simulation artifacts                                        시뮬레이션 아티팩트 방지를 위해 시작 시간을 약간 랜덤화
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0.05));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.06));

    for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        // Set the default gateway for the UE                                                           UE의 기본 게이트웨이 설정
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ApplicationContainer clientApps;
            ApplicationContainer serverApps;
            Ptr<EpcTft> tft = Create<EpcTft>();

            if (!disableDl)
            {
                ++dlPort;

                NS_LOG_LOGIC("installing UDP DL app for UE " << u);                                     // UE에 대한 UDP DL 앱 설치
                UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
                clientApps.Add(dlClientHelper.Install(remoteHost));
                PacketSinkHelper dlPacketSinkHelper(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                serverApps.Add(dlPacketSinkHelper.Install(ue));

                EpcTft::PacketFilter dlpf;
                dlpf.localPortStart = dlPort;
                dlpf.localPortEnd = dlPort;
                tft->Add(dlpf);
            }

            if (!disableUl)
            {
                ++ulPort;

                NS_LOG_LOGIC("installing UDP UL app for UE " << u);                                     // UE에 대한 UDP UL 앱 설치
                UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
                clientApps.Add(ulClientHelper.Install(ue));
                PacketSinkHelper ulPacketSinkHelper(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), ulPort));
                serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

                EpcTft::PacketFilter ulpf;
                ulpf.remotePortStart = ulPort;
                ulpf.remotePortEnd = ulPort;
                tft->Add(ulpf);
            }

            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
            clientApps.Stop(simTime);

        } // end for b
    }

    // Add X2 interface                                                                                 X2 인터페이스 추가
    lteHelper->AddX2Interface(enbNodes);

    // X2-based Handover                                                                                X2 기반 핸드오버
    lteHelper->HandoverRequest(MilliSeconds(300),
                               ueLteDevs.Get(0),
                               enbLteDevs.Get(0),
                               enbLteDevs.Get(1));

    // Uncomment to enable PCAP tracing                                                                 PACP 추적 활성화를 원하면 주석 해제
    // p2ph.EnablePcapAll("lena-x2-handover");

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    lteHelper->EnablePdcpTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));

    // connect custom trace sinks for RRC connection establishment and handover notification            RRC 연결 설정 및 핸드오버 알림에 사용자 정의 트레이스 싱크 연결
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkUe));

    // Hook a trace sink (the same one) to the four handover failure traces                             핸드오버 실패 트레이스 싱크 연결
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureNoPreamble",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureMaxRach",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureLeaving",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureJoining",
                    MakeCallback(&NotifyHandoverFailure));

    Simulator::Stop(simTime + MilliSeconds(20));
    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
