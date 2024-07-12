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

#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include <ns3/buildings-helper.h>
#include <ns3/log.h>
#include <ns3/spectrum-module.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaFrequencyReuse");

void
PrintGnuplottableUeListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                                   // 파일을 열 수 없습니다.
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
        NS_LOG_ERROR("Can't open file " << filename);                                                   // 파일을 열 수 없습니다.
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
    double simTime = 2.500;
    bool generateSpectrumTrace = false;
    bool generateRem = false;
    int32_t remRbId = -1;
    uint16_t bandwidth = 25;
    double distance = 1000;
    Box macroUeBox =
        Box(-distance * 0.5, distance * 1.5, -distance * 0.5, distance * 1.5, 1.5, 1.5);

    // Command line arguments   명령행 인수 처리
    CommandLine cmd(__FILE__);
    cmd.AddValue("numberOfUes", "Number of random UEs", numberOfRandomUes);                             // 랜덤 UE의 수
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);                  // 시뮬레이션의 총 시간 (초 단위)
    cmd.AddValue("generateSpectrumTrace",
                 "if true, will generate a Spectrum Analyzer trace",                                    // true로 설정하면 Spectrum Analyzer 추적을 생성합니다.
                 generateSpectrumTrace);
    cmd.AddValue("generateRem",
                 "if true, will generate a REM and then abort the simulation",                          // true로 설정하면 REM을 생성하고 시뮬레이션을 중단합니다.
                 generateRem);
    cmd.AddValue("remRbId",
                 "Resource Block Id, for which REM will be generated,"
                 "default value is -1, what means REM will be averaged from all RBs",                   // REM을 생성할 리소스 블록 ID, 기본값은 -1로
                 remRbId);                                                                              // 모든 RB에서 REM을 평균화합니다.
    cmd.AddValue("runId", "runId", runId);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(runId);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    // Create Nodes: eNodeB and UE                                                                      노드 생성: eNB와 UE
    NodeContainer enbNodes;
    NodeContainer centerUeNodes;
    NodeContainer edgeUeNodes;
    NodeContainer randomUeNodes;
    enbNodes.Create(3);
    centerUeNodes.Create(3);
    edgeUeNodes.Create(3);
    randomUeNodes.Create(numberOfRandomUes);

    /*   the topology is the following:                                                                 네트워크 구성은 다음과 같습니다.
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

    // Install Mobility Model                                                                           이동성 모델 설치
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0.0, 0.0, 0.0));                                                       // eNB1
    enbPositionAlloc->Add(Vector(distance, 0.0, 0.0));                                                  // eNB2
    enbPositionAlloc->Add(Vector(distance * 0.5, distance * 0.866, 0.0));                               // eNB3
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(enbPositionAlloc);
    mobility.Install(enbNodes);

    Ptr<ListPositionAllocator> edgeUePositionAlloc = CreateObject<ListPositionAllocator>();
    edgeUePositionAlloc->Add(Vector(distance * 0.5, distance * 0.28867, 0.0));                          // edgeUE1
    edgeUePositionAlloc->Add(Vector(distance * 0.5, distance * 0.28867, 0.0));                          // edgeUE2
    edgeUePositionAlloc->Add(Vector(distance * 0.5, distance * 0.28867, 0.0));                          // edgeUE3
    mobility.SetPositionAllocator(edgeUePositionAlloc);
    mobility.Install(edgeUeNodes);

    Ptr<ListPositionAllocator> centerUePositionAlloc = CreateObject<ListPositionAllocator>();
    centerUePositionAlloc->Add(Vector(0.0, 0.0, 0.0));                                                  // centerUE1
    centerUePositionAlloc->Add(Vector(distance, 0.0, 0.0));                                             // centerUE2
    centerUePositionAlloc->Add(Vector(distance * 0.5, distance * 0.866, 0.0));                          // centerUE3
    mobility.SetPositionAllocator(centerUePositionAlloc);
    mobility.Install(centerUeNodes);

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

    // Create Devices and install them in the Nodes (eNB and UE)                                        디바이스 생성 및 노드(eNB 및 UE)에 디바이스 설치
    NetDeviceContainer enbDevs;
    NetDeviceContainer edgeUeDevs;
    NetDeviceContainer centerUeDevs;
    NetDeviceContainer randomUeDevs;
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetSchedulerAttribute("UlCqiFilter", EnumValue(FfMacScheduler::PUSCH_UL_CQI));
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));

    std::string frAlgorithmType = lteHelper->GetFfrAlgorithmType();
    NS_LOG_DEBUG("FrAlgorithmType: " << frAlgorithmType);

    if (frAlgorithmType == "ns3::LteFrHardAlgorithm")
    {
        // Nothing to configure here in automatic mode                                                  자동 모드에서는 여기에서 구성할 항목이 없습니다.
    }
    else if (frAlgorithmType == "ns3::LteFrStrictAlgorithm")
    {
        lteHelper->SetFfrAlgorithmAttribute("RsrqThreshold", UintegerValue(32));
        lteHelper->SetFfrAlgorithmAttribute("CenterPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB_6));
        lteHelper->SetFfrAlgorithmAttribute("EdgePowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(0));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));

        // ns3::LteFrStrictAlgorithm works with Absolute Mode Uplink Power Control  
        Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));
    }
    else if (frAlgorithmType == "ns3::LteFrSoftAlgorithm")
    {
        lteHelper->SetFfrAlgorithmAttribute("AllowCenterUeUseEdgeSubBand", BooleanValue(true));
        lteHelper->SetFfrAlgorithmAttribute("RsrqThreshold", UintegerValue(25));
        lteHelper->SetFfrAlgorithmAttribute("CenterPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB_6));
        lteHelper->SetFfrAlgorithmAttribute("EdgePowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(0));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));

        // ns3::LteFrSoftAlgorithm works with Absolute Mode Uplink Power Control
        Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));
    }
    else if (frAlgorithmType == "ns3::LteFfrSoftAlgorithm")
    {
        lteHelper->SetFfrAlgorithmAttribute("CenterRsrqThreshold", UintegerValue(30));
        lteHelper->SetFfrAlgorithmAttribute("EdgeRsrqThreshold", UintegerValue(25));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB_6));
        lteHelper->SetFfrAlgorithmAttribute(
            "MediumAreaPowerOffset",
            UintegerValue(LteRrcSap::PdschConfigDedicated::dB_1dot77));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(1));
        lteHelper->SetFfrAlgorithmAttribute("MediumAreaTpc", UintegerValue(2));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));

        // ns3::LteFfrSoftAlgorithm works with Absolute Mode Uplink Power Control
        Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));
    }
    else if (frAlgorithmType == "ns3::LteFfrEnhancedAlgorithm")
    {
        lteHelper->SetFfrAlgorithmAttribute("RsrqThreshold", UintegerValue(25));
        lteHelper->SetFfrAlgorithmAttribute("DlCqiThreshold", UintegerValue(10));
        lteHelper->SetFfrAlgorithmAttribute("UlCqiThreshold", UintegerValue(10));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB_6));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaPowerOffset",
                                            UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));
        lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(0));
        lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));

        // ns3::LteFfrEnhancedAlgorithm works with Absolute Mode Uplink Power Control
        Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));
    }
    else if (frAlgorithmType == "ns3::LteFfrDistributedAlgorithm")
    {
        NS_FATAL_ERROR("ns3::LteFfrDistributedAlgorithm not supported in this example. Please run "
                       "lena-distributed-ffr");
    }
    else
    {
        lteHelper->SetFfrAlgorithmType("ns3::LteFrNoOpAlgorithm");
    }

    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(1));
    enbDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(0)));

    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(2));
    enbDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(1)));

    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(3));
    enbDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(2)));

    // FR algorithm reconfiguration if needed                                                           필요한 경우 FR 알고리즘 재구성
    PointerValue tmp;
    enbDevs.Get(0)->GetAttribute("LteFfrAlgorithm", tmp);
    Ptr<LteFfrAlgorithm> ffrAlgorithm = DynamicCast<LteFfrAlgorithm>(tmp.GetObject());
    ffrAlgorithm->SetAttribute("FrCellTypeId", UintegerValue(1));

    // Install Ue Device                                                                                Ue 디바이스 설치
    edgeUeDevs = lteHelper->InstallUeDevice(edgeUeNodes);
    centerUeDevs = lteHelper->InstallUeDevice(centerUeNodes);
    randomUeDevs = lteHelper->InstallUeDevice(randomUeNodes);

    // Attach edge UEs to eNbs                                                                          엣지 UE를 eNbs에 연결
    for (uint32_t i = 0; i < edgeUeDevs.GetN(); i++)
    {
        lteHelper->Attach(edgeUeDevs.Get(i), enbDevs.Get(i));
    }
    // Attach center UEs to eNbs                                                                        센터 UE를 eNbs에 연결
    for (uint32_t i = 0; i < centerUeDevs.GetN(); i++)
    {
        lteHelper->Attach(centerUeDevs.Get(i), enbDevs.Get(i));
    }

    // Attach UE to a eNB                                                                               UE를 eNB에 연결
    lteHelper->AttachToClosestEnb(randomUeDevs, enbDevs);

    // Activate a data radio bearer                                                                     데이터 라디오 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(edgeUeDevs, bearer);
    lteHelper->ActivateDataRadioBearer(centerUeDevs, bearer);
    lteHelper->ActivateDataRadioBearer(randomUeDevs, bearer);

    // Spectrum analyzer                                                                                스펙트럼 분석기
    NodeContainer spectrumAnalyzerNodes;
    spectrumAnalyzerNodes.Create(1);
    SpectrumAnalyzerHelper spectrumAnalyzerHelper;

    if (generateSpectrumTrace)
    {
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
        // position of Spectrum Analyzer                                                                스펙트럼 분석기의 위치
        // positionAlloc->Add (Vector (0.0, 0.0, 0.0));                                                 // eNB1
        // positionAlloc->Add (Vector (distance,  0.0, 0.0));                                           // eNB2
        positionAlloc->Add(Vector(distance * 0.5, distance * 0.866, 0.0));                              // eNB3

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
                                               DoubleValue(1e-15));                                     // -120 dBm/Hz
        spectrumAnalyzerHelper.EnableAsciiAll("spectrum-analyzer-output");
        spectrumAnalyzerHelper.Install(spectrumAnalyzerNodes);
    }

    Ptr<RadioEnvironmentMapHelper> remHelper;
    if (generateRem)
    {
        PrintGnuplottableEnbListToFile("enbs.txt");
        PrintGnuplottableUeListToFile("ues.txt");

        remHelper = CreateObject<RadioEnvironmentMapHelper>();
        remHelper->SetAttribute("ChannelPath", StringValue("/ChannelList/0"));
        remHelper->SetAttribute("OutputFile", StringValue("lena-frequency-reuse.rem"));
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
        // simulation will stop right after the REM has been generated                                  시뮬레이션은 REM이 생성된 직후에 중지됩니다.
    }
    else
    {
        Simulator::Stop(Seconds(simTime));
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
