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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include <ns3/buildings-helper.h>
#include <ns3/string.h>

#include <fstream>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:                          템플릿 기본 속성 파일을 저장하려면 다음과 같이 실행하세요:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file                                  이전에 생성된 기본 속성 파일을 로드하려면
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    // ConfigStore inputConfig;
    // inputConfig.ConfigureDefaults ();

    // parse again so you can override default values from the command line                 명령 줄에서 기본값을 재정의할 수 있도록 다시 파싱합니다
    // cmd.Parse (argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    // Uncomment to enable logging                                                          로깅을 활성화하려면 주석을 해제하세요
    // lteHelper->EnableLogComponents ();

    lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));

    std::ifstream ifTraceFile;
    ifTraceFile.open("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad",
                     std::ifstream::in);
    if (ifTraceFile.good())
    {
        // script launched by test.py                                                       test.py에 의해 실행된 스크립트
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
    else
    {
        // script launched as an example                                                    예제로서 실행된 스크립트
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }

    // these parameters have to be set only in case of the trace format                     이 매개변수들은 트레이스 형식이 표준과 다른 경우에만 설정해야 합니다.
    // differs from the standard one, that is
    // - 10 seconds length trace                                                            -10초 길이의 트레이스
    // - 10,000 samples                                                                     -10,000개의 샘플
    // - 0.5 seconds for window size                                                        -0.5초의 창 크기
    // - 100 RB                                                                             - 100 RB
    lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10.0)));
    lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
    lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
    lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));

    // Create Nodes: eNodeB and UE                                                          노드 생성: eNodeB 및 UE
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

    // Create Devices and install them in the Nodes (eNB and UE)                            장치 생성 및 노드(eNB 및 UE)에 설치
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB                                                                 UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate an EPS bearer                                                               EPS 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.005));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
