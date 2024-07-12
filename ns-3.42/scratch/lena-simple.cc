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

int
main(int argc, char* argv[])
{
    Time simTime = MilliSeconds(1050);                                                      // 시뮬레이션 시간 설정(1050ms)
    bool useCa = false;                                                                     // 캐리어 애그리게이션 사용 여부(T/F)

    CommandLine cmd(__FILE__);                                                              // 명령줄 인수 파싱을 위한 설정
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:                          템플릿 기본 속성 파일을 저장하려면 다음과 같이 실행하시오:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file                                  이전에 생성된 기본 속성 파일을 로드하려면...
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    ConfigStore inputConfig;                                                                // ConfigStore 객체 생성 및 기본값 설정
    inputConfig.ConfigureDefaults();

    // Parse again so you can override default values from the command line                 명령줄 인수 다시 파싱(기본값 덮어쓰기)
    cmd.Parse(argc, argv);

    if (useCa)                                                                              // 캐리어 애그리게이션이 사용되면 관련 설정을 적용
    {
        Config::SetDefault("ns3::LteHelper::UseCa", BooleanValue(useCa));
        Config::SetDefault("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue(2));
        Config::SetDefault("ns3::LteHelper::EnbComponentCarrierManager",
                           StringValue("ns3::RrComponentCarrierManager"));
    }

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();                                   // LteHelper 객체 생성

    // Uncomment to enable logging
    //  lteHelper->EnableLogComponents ();                                                  // 로깅 활성화(주석 해제 시)

    // Create Nodes: eNodeB and UE                                                          eNB 및 UE 노드 생성
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);

    // Install Mobility Model                                                               이동성 모델 설치
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                            디바이스 생성 및 노드에 설치 (eNB 및 UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    // Default scheduler is PF, uncomment to use RR                                         기본 스케줄러는 PF (Proportional Fair), RR(Round Robin)을 활성화(주석 해제 시)
    // lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB                                                                 UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate a data radio bearer                                                         데이터 라디오 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);
    lteHelper->EnableTraces();

    Simulator::Stop(simTime);                                                               // 시뮬레이션 시간 설정 및 실행
    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();                                                                   // 시뮬레이터 종료
    return 0;
}
