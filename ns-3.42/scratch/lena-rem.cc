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
#include "ns3/spectrum-module.h"
#include <ns3/buildings-helper.h>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:                      기본 속성 파일을 저장하기 위한 명령어 예제
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file                              이전에 생성된 기본 속성 파일을 불러오기 위한 명령어 예제
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // Parse again so you can override default values from the command line             명령행을 통해 기본값을 재정의할 수 있도록 다시 파싱
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    // Uncomment to enable logging                                                      로깅을 활성화하려면 주석 해제
    // lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE                                                      노드 생성: eNB와 UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);

    // Install Mobility Model                                                           이동성 모델 설치
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                        장치 생성 및 노드에 설치(eNB와 UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    // Default scheduler is PF, uncomment to use RR                                     기본 스케줄러는 PF이며, RR을 사용하려면 주석 해제
    // lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB                                                             UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate an EPS bearer                                                           EPS 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    // Configure Radio Environment Map (REM) output                                     라디오 환경 맵(REM) 설정
    // for LTE-only simulations always use /ChannelList/0 which is the downlink channel LTE 전용 시뮬레이션에서는 항상 /ChannelList/0을 사용합니다 (다운링크 채널)
    Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper>();
    remHelper->SetAttribute("ChannelPath", StringValue("/ChannelList/0"));
    remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
    remHelper->SetAttribute("XMin", DoubleValue(-400.0));
    remHelper->SetAttribute("XMax", DoubleValue(400.0));
    remHelper->SetAttribute("YMin", DoubleValue(-300.0));
    remHelper->SetAttribute("YMax", DoubleValue(300.0));
    remHelper->SetAttribute("Z", DoubleValue(0.0));
    remHelper->Install();

    // here's a minimal gnuplot script that will plot the above:                        다음은 위에서 생성한 최소한의 gunplot 스크립트입니다:
    //
    // set view map;
    // set term x11;
    // set xlabel "X"
    // set ylabel "Y"
    // set cblabel "SINR (dB)"
    // plot "rem.out" using ($1):($2):(10*log10($4)) with image

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
