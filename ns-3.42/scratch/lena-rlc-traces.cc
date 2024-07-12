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
 */

#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;                                                                        // 기본 설정 값으로 ConfigStroe 초기화
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                         명령행 인자를 다시 파싱하여 기본 값을 재정의 할 수 있도록 함
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();                                           // LTE Helper 객체 생성

    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));// Pathloss 모델 설정 (Friis 스펙트럼 Propagation Loss Model)
    // Enable LTE log components                                                                    LTE 로그 컴포넌트 활성화 (주석을 해제하여 활성화)
    // lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE                                                                  노드 생성: eNB와 UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(3);

    // Install Mobility Model                                                                       이동성 모델 설치
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                                    장치 생성 및 노드에 설치 (eNB와 UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB                                                                         UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate an EPS bearer                                                                       EPS 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.5));                                                                  // 시뮬레이션 종료 시간 설정

    lteHelper->EnablePhyTraces();                                                                   // 물리 트레이스 활성화
    lteHelper->EnableMacTraces();                                                                   // MAC 트레이스 활성화
    lteHelper->EnableRlcTraces();                                                                   // RLC 트레이스 활성화

    double distance_temp[] = {1000, 1000, 1000};                                                    // 사용자 위치 설정
    std::vector<double> userDistance;
    userDistance.assign(distance_temp, distance_temp + 3);
    for (int i = 0; i < 3; i++)
    {
        Ptr<ConstantPositionMobilityModel> mm =
            ueNodes.Get(i)->GetObject<ConstantPositionMobilityModel>();
        mm->SetPosition(Vector(userDistance[i], 0.0, 0.0));
    }

    Simulator::Run();                                                                               // 시뮬레이션 실행

    // Uncomment to show available paths                                                            사용 가능한 경로를 보려면 주석을 해제하세요
    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();                                                                           // 시뮬레이션 종료

    return 0;
}
