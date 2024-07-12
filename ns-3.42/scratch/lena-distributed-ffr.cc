/*
 * Copyright (c) 2014 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/spectrum-module.h"
#include <ns3/buildings-helper.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaDistributedFrequencyReuse");

void
PrintGnuplottableUeListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                               // 파일을 열 수 없습니다.
        return;
    }
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteUeNetDevice> uedev = node->GetDevice(j)->GetObject<LteUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << uedev->GetImsi() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps "
                           "0.3 lc rgb \"grey\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

void
PrintGnuplottableEnbListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                               // 파일을 열 수 없습니다.
        return;
    }
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteEnbNetDevice> enbdev = node->GetDevice(j)->GetObject<LteEnbNetDevice>();
            if (enbdev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << enbdev->GetCellId() << "\" at " << pos.x << ","
                        << pos.y
                        << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 "
                           "ps 0.3 lc rgb \"white\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(true));
    Config::SetDefault("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue(true));

    // Uplink Power Control
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::ClosedLoop", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));

    uint32_t runId = 3;
    uint16_t numberOfRandomUes = 0;
    double simTime = 5.000;
    bool generateSpectrumTrace = false;
    bool generateRem = false;
    int32_t remRbId = -1;
    uint16_t bandwidth = 25;
    double distance = 1000;
    Box macroUeBox =
        Box(-distance * 0.5, distance * 1.5, -distance * 0.5, distance * 1.5, 1.5, 1.5);

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numberOfUes", "Number of UEs", numberOfRandomUes);                                // UE의 수
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);              // 시뮬레이션 총 시간(초)
    cmd.AddValue("generateSpectrumTrace",
                 "if true, will generate a Spectrum Analyzer trace",
                 generateSpectrumTrace);                                                            // true로 설정 시 Spectrum Analyzer 추적 파일을 생성합니다.
    cmd.AddValue("generateRem",
                 "if true, will generate a REM and then abort the simulation",
                 generateRem);                                                                      // true로 설정 시 REM을 생성하고 시뮬레이션을 중단합니다.
    cmd.AddValue("remRbId",
                 "Resource block Id, for which REM will be generated,"
                 "default value is -1, what means REM will be averaged from all RBs",               // REM을 생성할 Resource Block Id입니다.
                 remRbId);                                                                          // 기본 값은 -1로 모든 RBs에서 평균을 계산합니다.
    cmd.AddValue("runId", "runId", runId);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(runId);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm"); // disable automatic handover 자동 핸드오버 비활성화

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost                                                                   단일 RemoteHost 생성
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet                                                                          인터넷 생성
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Routing of the Internet Host (towards the LTE network)                                       인터넷 호스트의 경로 설정 (LTE 네트워크로)
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    // interface 0 is localhost, 1 is the p2p device                                                인터페이스 0은 localhost, 1은 p2p장치입니다.
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Create Nodes: eNodeB and UE                                                                  eNB 및 UE 노드 생성
    NodeContainer enbNodes;
    NodeContainer randomUeNodes;
    enbNodes.Create(3);
    randomUeNodes.Create(numberOfRandomUes);

    /*   the topology is the following:                                                             다음은 토폴로지이다.
     *                 eNB3
     *                /     \
     *               /       \
     *              /         \
     *             /           \
     *   distance /             \ distance
     *           /      UEs      \
     *          /                 \
     *         /                   \
     *        /                     \
     *       /                       \
     *   eNB1-------------------------eNB2
     *                  distance
     */

    // Install Mobility Model                                                                       이동성 모델 설치
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0.0, 0.0, 0.0));                                                   // eNB1 위치
    enbPositionAlloc->Add(Vector(distance, 0.0, 0.0));                                              // eNB2 위치
    enbPositionAlloc->Add(Vector(distance * 0.5, distance * 0.866, 0.0));                           // eNB3 위치
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(enbPositionAlloc);
    mobility.Install(enbNodes);

    Ptr<RandomBoxPositionAllocator> randomUePositionAlloc =
        CreateObject<RandomBoxPositionAllocator>();
    Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable>();
    xVal->SetAttribute("Min", DoubleValue(macroUeBox.xMin));
    xVal->SetAttribute("Max", DoubleValue(macroUeBox.xMax));
    randomUePositionAlloc->SetAttribute("X", PointerValue(xVal));
    Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable>();
    yVal->SetAttribute("Min", DoubleValue(macroUeBox.yMin));
    yVal->SetAttribute("Max", DoubleValue(macroUeBox.yMax));
    randomUePositionAlloc->SetAttribute("Y", PointerValue(yVal));
    Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable>();
    zVal->SetAttribute("Min", DoubleValue(macroUeBox.zMin));
    zVal->SetAttribute("Max", DoubleValue(macroUeBox.zMax));
    randomUePositionAlloc->SetAttribute("Z", PointerValue(zVal));
    mobility.SetPositionAllocator(randomUePositionAlloc);
    mobility.Install(randomUeNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                                    장치 생성 및 노드에 설치(eNodeB 및 UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer randomUeDevs;
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(true));

    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));

    lteHelper->SetFfrAlgorithmType("ns3::LteFfrDistributedAlgorithm");
    lteHelper->SetFfrAlgorithmAttribute("CalculationInterval", TimeValue(MilliSeconds(200)));
    lteHelper->SetFfrAlgorithmAttribute("RsrpDifferenceThreshold", UintegerValue(5));
    lteHelper->SetFfrAlgorithmAttribute("RsrqThreshold", UintegerValue(25));
    lteHelper->SetFfrAlgorithmAttribute("EdgeRbNum", UintegerValue(6));
    lteHelper->SetFfrAlgorithmAttribute("CenterPowerOffset",
                                        UintegerValue(LteRrcSap::PdschConfigDedicated::dB_3));
    lteHelper->SetFfrAlgorithmAttribute("EdgePowerOffset",
                                        UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));

    lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(0));
    lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));

    // ns3::LteFfrDistributedAlgorithm works with Absolute Mode Uplink Power Control                둘이 함께 작동함
    Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    randomUeDevs = lteHelper->InstallUeDevice(randomUeNodes);

    // Add X2 interface                                                                             X2 인터페이스 추가
    lteHelper->AddX2Interface(enbNodes);

    NodeContainer ueNodes;
    ueNodes.Add(randomUeNodes);
    NetDeviceContainer ueDevs;
    ueDevs.Add(randomUeDevs);

    // Install the IP stack on the UEs                                                              UEs에 IP 스택 설치
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    // Attach a UE to a eNB                                                                         UE를 eNodeB에 연결
    lteHelper->AttachToClosestEnb(ueDevs, enbDevs);

    // Install and start applications on UEs and remote host                                        UEs 및 원격 호스트에 응용 프로그램 설치 및 시작
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    // randomize a bit start times to avoid simulation artifacts                                    시뮬레이션 아티팩트(Ex: 패킷 전송이 정확히 동시에 발생하여 버퍼 
    // (e.g., buffer overflows due to packet transmissions happening                                오버플로우가 발생하는 것)를 피하기 위해 시작 시간을 약간 랜덤화
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        // Set the default gateway for the UE                                                       UE에 대한 기본 게이트웨이 설정
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < 1; ++b)
        {
            ++dlPort;
            ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            dlClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1.0)));
            clientApps.Add(dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ue));

            UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
            ulClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            ulClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1.0)));
            clientApps.Add(ulClientHelper.Install(ue));
            PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

            Ptr<EpcTft> tft = Create<EpcTft>();
            EpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            tft->Add(dlpf);
            EpcTft::PacketFilter ulpf;
            ulpf.remotePortStart = ulPort;
            ulpf.remotePortEnd = ulPort;
            tft->Add(ulpf);
            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
        }
    }

    // Spectrum analyzer                                                                            스펙트럼 분석기
    NodeContainer spectrumAnalyzerNodes;
    spectrumAnalyzerNodes.Create(1);
    SpectrumAnalyzerHelper spectrumAnalyzerHelper;

    if (generateSpectrumTrace)
    {
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
        // position of Spectrum Analyzer                                                            스펙트럼 분석기의 위치
        positionAlloc->Add(Vector(0.0, 0.0, 0.0));                                                  // eNB1 위치
        //      positionAlloc->Add (Vector (distance,  0.0, 0.0));                                  // eNB2 위치
        //      positionAlloc->Add (Vector (distance*0.5, distance*0.866, 0.0));                    // eNB3 위치

        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(spectrumAnalyzerNodes);

        Ptr<LteSpectrumPhy> enbDlSpectrumPhy = enbDevs.Get(0)
                                                   ->GetObject<LteEnbNetDevice>()
                                                   ->GetPhy()
                                                   ->GetDownlinkSpectrumPhy()
                                                   ->GetObject<LteSpectrumPhy>();
        Ptr<SpectrumChannel> dlChannel = enbDlSpectrumPhy->GetChannel();

        spectrumAnalyzerHelper.SetChannel(dlChannel);
        Ptr<SpectrumModel> sm = LteSpectrumValueHelper::GetSpectrumModel(100, bandwidth);
        spectrumAnalyzerHelper.SetRxSpectrumModel(sm);
        spectrumAnalyzerHelper.SetPhyAttribute("Resolution", TimeValue(MicroSeconds(10)));
        spectrumAnalyzerHelper.SetPhyAttribute("NoisePowerSpectralDensity",
                                               DoubleValue(1e-15));                                 // -120 dBm/Hz
        spectrumAnalyzerHelper.EnableAsciiAll("spectrum-analyzer-output");
        spectrumAnalyzerHelper.Install(spectrumAnalyzerNodes);
    }

    Ptr<RadioEnvironmentMapHelper> remHelper;
    if (generateRem)
    {
        PrintGnuplottableEnbListToFile("enbs.txt");
        PrintGnuplottableUeListToFile("ues.txt");

        remHelper = CreateObject<RadioEnvironmentMapHelper>();
        Ptr<LteSpectrumPhy> enbDlSpectrumPhy = enbDevs.Get(0)
                                                   ->GetObject<LteEnbNetDevice>()
                                                   ->GetPhy()
                                                   ->GetDownlinkSpectrumPhy()
                                                   ->GetObject<LteSpectrumPhy>();
        Ptr<SpectrumChannel> dlChannel = enbDlSpectrumPhy->GetChannel();
        uint32_t dlChannelId = dlChannel->GetId();
        NS_LOG_INFO("DL ChannelId: " << dlChannelId);
        remHelper->SetAttribute("Channel", PointerValue(dlChannel));
        remHelper->SetAttribute("OutputFile", StringValue("lena-distributed-ffr.rem"));
        remHelper->SetAttribute("XMin", DoubleValue(macroUeBox.xMin));
        remHelper->SetAttribute("XMax", DoubleValue(macroUeBox.xMax));
        remHelper->SetAttribute("YMin", DoubleValue(macroUeBox.yMin));
        remHelper->SetAttribute("YMax", DoubleValue(macroUeBox.yMax));
        remHelper->SetAttribute("Z", DoubleValue(1.5));
        remHelper->SetAttribute("XRes", UintegerValue(500));
        remHelper->SetAttribute("YRes", UintegerValue(500));

        if (remRbId >= 0)
        {
            remHelper->SetAttribute("UseDataChannel", BooleanValue(true));
            remHelper->SetAttribute("RbId", IntegerValue(remRbId));
        }

        remHelper->Install();
        // simulation will stop right after the REM has been generated                              시뮬레이션은 REM이 생성된 직후에 중지됩니다.
    }
    else
    {
        Simulator::Stop(Seconds(simTime));
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
