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

#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include <ns3/buildings-helper.h>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Change the position of a node.                                                                       노드의 위치를 변경합니다
 *
 * This function will move a node with a x coordinate greater than 10 m                                 이 함수는 x 좌표가 10m 보다 큰 노드를 x=5 m로,
 * to a x equal to 5 m, and less than or equal to 10 m to 10 Km                                         10m 이하인 노드는 x=10km로 이동시킵니다.
 *
 * \param node The node to move.                                                                        이동할 노드입니다.
 */
static void
ChangePosition(Ptr<Node> node)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();

    if (pos.x <= 10.0)
    {
        pos.x = 100000.0; // force CQI to 0                                                             CQI를 0으로 강제 설정
    }
    else
    {
        pos.x = 5.0;
    }
    mobility->SetPosition(pos);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:                                      기본 속성 파일 템플릿을 저장하려면 다음과 같이 실행하십시오:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file                                              이전에 생성한 기본 속성 파일을 로드하려면:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                             명령줄에서 기본 값을 재설정할 수 있도록 다시 파싱합니다.
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    // Uncomment to enable logging                                                                      로그 기록을 활성화하려면 주석을 해제하십시오
    // lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE                                                                      노드 생성: eNodeB와 UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);

    // Install Mobility Model                                                                           이동성 모델 설치
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                                        장치 생성 및 노드(eNB 및 UE)에 설치
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    //   lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");         
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetSchedulerAttribute("CqiTimerThreshold", UintegerValue(3));
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    lteHelper->EnableRlcTraces();
    lteHelper->EnableMacTraces();

    // Attach a UE to a eNB                                                                             UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    Simulator::Schedule(Seconds(0.010), &ChangePosition, ueNodes.Get(0));
    Simulator::Schedule(Seconds(0.020), &ChangePosition, ueNodes.Get(0));

    // Activate a data radio bearer                                                                     데이터 라디오 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.030));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
