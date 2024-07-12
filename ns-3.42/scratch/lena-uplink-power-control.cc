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

using namespace ns3;

/*
 * This example show how to configure and how Uplink Power Control works.                   이 예제는 Uplink Power Control 설정과 동작을 보여줍니다.
 */

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(false));

    double eNbTxPower = 30;
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(eNbTxPower));                 // eNB의 전송 전력 설정
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));                        // UE의 전송 전력 설정
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(true));      // Uplink Power Control 활성화 설정

    Config::SetDefault("ns3::LteUePowerControl::ClosedLoop", BooleanValue(true));           // Closed-loop 방식의 Uplink Power Control 설정
    Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(true));  // 적산 기능 활성화 설정
    Config::SetDefault("ns3::LteUePowerControl::Alpha", DoubleValue(1.0));                  // 적산 가중치 설정

    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    uint16_t bandwidth = 25;                                                                // 대역폭 설정
    double d1 = 0;

    // Create Nodes: eNodeB and UE                                                          노드 생성: eNB와 UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);
    NodeContainer allNodes = NodeContainer(enbNodes, ueNodes);

    /*   the topology is the following:                                                     다음과 같은 토폴로지를 가집니다:
     *
     *   eNB1-------------------------UE
     *                  d1
     */

    // Install Mobility Model                                                               이동성 모델 설치
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));                                              // eNB1 위치
    positionAlloc->Add(Vector(d1, 0.0, 0.0));                                               // UE1 위치

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(allNodes);

    // Create Devices and install them in the Nodes (eNB and UE)                            장치 생성 및 노드에 설치(eNB와 UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));              // 다운링크 대역폭 설정
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));              // 업링크 대역폭 설정

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB                                                                 UE를 eNB에 연결
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate a data radio bearer                                                         데이터 무선 베어러 활성화
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.500));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
