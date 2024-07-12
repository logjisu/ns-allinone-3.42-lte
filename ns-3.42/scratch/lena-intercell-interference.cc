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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/radio-bearer-stats-calculator.h"

#include <iomanip>
#include <string>

using namespace ns3;

/**
 * This simulation script creates two eNodeBs and drops randomly several UEs in                     이 시뮬레이션 스크립트는 두 개의 eNB를 생성하고 각각 주변의 원형 영역에
 * a disc around them (same number on both). The number of UEs , the radius of                      임의의 UE를 배치합니다.
 * that disc and the distance between the eNodeBs can be configured.                                UE의 수, 원의 반지름 및 eNB 간 거리는 설정할 수 있습니다.
 */
int
main(int argc, char* argv[])
{
    double enbDist = 100.0;                                                                         // 두 eNB 사이의 거리
    double radius = 50.0;                                                                           // 각 eNB 주변에 배치될 UE의 반경
    uint32_t numUes = 1;                                                                            // 각 eNB에 연결될 UE의 수
    double simTime = 1.0;                                                                           // 시뮬레이션의 총 시간(초)

    CommandLine cmd(__FILE__);
    cmd.AddValue("enbDist", "distance between the two eNBs", enbDist);                              // 두 eNB 사이의 거리
    cmd.AddValue("radius", "the radius of the disc where UEs are placed around an eNB", radius);    // eNB 주변의 UE가 배치될 원의 반경
    cmd.AddValue("numUes", "how many UEs are attached to each eNB", numUes);                        // 각 eNB에 연결될 UE의 수
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);              // 시뮬레이션의 총 시간 (초)
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                         명령행 인수를 통해 기본 값들을 재설정할 수 있도록 다시 파싱합니다.
    cmd.Parse(argc, argv);

    // determine the string tag that identifies this simulation run                                 시뮬레이션 실행 태그 생성: 이 태그는 모든 파일명에 추가됩니다.
    // this tag is then appended to all filenames

    UintegerValue runValue;
    GlobalValue::GetValueByName("RngRun", runValue);

    std::ostringstream tag;
    tag << "_enbDist" << std::setw(3) << std::setfill('0') << std::fixed << std::setprecision(0)
        << enbDist << "_radius" << std::setw(3) << std::setfill('0') << std::fixed
        << std::setprecision(0) << radius << "_numUes" << std::setw(3) << std::setfill('0')
        << numUes << "_rngRun" << std::setw(3) << std::setfill('0') << runValue.Get();

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));

    // Create Nodes: eNodeB and UE                                                                  노드 생성: eNodeB 및 UE
    NodeContainer enbNodes;
    NodeContainer ueNodes1;
    NodeContainer ueNodes2;
    enbNodes.Create(2);
    ueNodes1.Create(numUes);
    ueNodes2.Create(numUes);

    // Position of eNBs                                                                             eNB 위치 설정
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(enbDist, 0.0, 0.0));
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(positionAlloc);
    enbMobility.Install(enbNodes);

    // Position of UEs attached to eNB 1                                                            eNB 1에 연결된 UE의 위치 설정
    MobilityHelper ue1mobility;
    ue1mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                     "X",
                                     DoubleValue(0.0),
                                     "Y",
                                     DoubleValue(0.0),
                                     "rho",
                                     DoubleValue(radius));
    ue1mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ue1mobility.Install(ueNodes1);

    // Position of UEs attached to eNB 2                                                            eNB 2에 연결된 UE의 위치 설정
    MobilityHelper ue2mobility;
    ue2mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                     "X",
                                     DoubleValue(enbDist),
                                     "Y",
                                     DoubleValue(0.0),
                                     "rho",
                                     DoubleValue(radius));
    ue2mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ue2mobility.Install(ueNodes2);

    // Create Devices and install them in the Nodes (eNB and UE)                                    장치 생성 및 노드에 설치: eNB 및 UE
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs1;
    NetDeviceContainer ueDevs2;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs1 = lteHelper->InstallUeDevice(ueNodes1);
    ueDevs2 = lteHelper->InstallUeDevice(ueNodes2);

    // Attach UEs to a eNB                                                                          UE를 eNB에 연결
    lteHelper->Attach(ueDevs1, enbDevs.Get(0));
    lteHelper->Attach(ueDevs2, enbDevs.Get(1));

    // Activate a data radio bearer each UE                                                         각 UE에 데이터 무선 전송매체 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs1, bearer);
    lteHelper->ActivateDataRadioBearer(ueDevs2, bearer);

    Simulator::Stop(Seconds(simTime));

    // Insert RLC Performance Calculator                                                            RLC 성능 계산기 삽입
    std::string dlOutFname = "DlRlcStats";
    dlOutFname.append(tag.str());
    std::string ulOutFname = "UlRlcStats";
    ulOutFname.append(tag.str());

    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
