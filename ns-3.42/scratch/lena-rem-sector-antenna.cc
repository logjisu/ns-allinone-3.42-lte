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
 * Author: Jaume Nin <jnin@cttc.es>
 */

#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/radio-environment-map-helper.h>

#include <iomanip>
#include <string>
#include <vector>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    cmd.Parse(argc, argv);

    // Geometry of the scenario (in meters)                                                     시나리오 기하하적 설정(미터 단위)
    // Assume squared building                                                                  정사각형 건물을 가정합니다.
    double nodeHeight = 1.5;                                                                    // 노드 높이
    double roomHeight = 3;                                                                      // 방 높이
    double roomLength = 500;                                                                    // 방 길이
    uint32_t nRooms = 2;                                                                        // 방 개수
    // Create one eNodeB per room + one 3 sector eNodeB (i.e. 3 eNodeB) + one regular eNodeB    방당 하나의 eNB를 생성하고 3개 섹터 eNB(3개의 eNB)와 일반 eNB 하나를 추가합니다.
    uint32_t nEnb = nRooms * nRooms + 4;                                                        // eNB 개수
    uint32_t nUe = 1;                                                                           // UE 개수

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    // lteHelper->EnableLogComponents ();
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));

    // Create Nodes: eNodeB and UE                                                              노드 생성: eNB 및 UE
    NodeContainer enbNodes;
    NodeContainer oneSectorNodes;
    NodeContainer threeSectorNodes;
    std::vector<NodeContainer> ueNodes;

    oneSectorNodes.Create(nEnb - 3);                                                            // 3개의 섹터를 제외한 eNB 노드 생성
    threeSectorNodes.Create(3);                                                                 // 3개의 섹터 eNB 노드 생성

    enbNodes.Add(oneSectorNodes);
    enbNodes.Add(threeSectorNodes);

    for (uint32_t i = 0; i < nEnb; i++)
    {
        NodeContainer ueNode;
        ueNode.Create(nUe);                                                                     // 각 eNB에 연결될 UE 노드 생성
        ueNodes.push_back(ueNode);
    }

    MobilityHelper mobility;
    std::vector<Vector> enbPosition;                                                            // eNB 위치 백터 배열
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    Ptr<Building> building;
    building = Create<Building>();
    building->SetBoundaries(
        Box(0.0, nRooms * roomLength, 0.0, nRooms * roomLength, 0.0, roomHeight));              // 건물 경계 설정
    building->SetBuildingType(Building::Residential);                                           // 건물 유형 설정
    building->SetExtWallsType(Building::ConcreteWithWindows);                                   // 외벽 타입 설정
    building->SetNFloors(1);                                                                    // 층 수 설정
    building->SetNRoomsX(nRooms);                                                               // X방 개수 설정
    building->SetNRoomsY(nRooms);                                                               // Y방 개수 설정
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");                            // 이동성 모델 설정
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);                                                         // 건물 헬퍼 설치
    uint32_t plantedEnb = 0;
    for (uint32_t row = 0; row < nRooms; row++)
    {
        for (uint32_t column = 0; column < nRooms; column++, plantedEnb++)
        {
            Vector v(roomLength * (column + 0.5), roomLength * (row + 0.5), nodeHeight);
            positionAlloc->Add(v);                                                              // 위치 할당기에 추가
            enbPosition.push_back(v);
            Ptr<MobilityModel> mmEnb = enbNodes.Get(plantedEnb)->GetObject<MobilityModel>();
            mmEnb->SetPosition(v);                                                              // eNB의 위치 설정
        }
    }

    // Add a 1-sector site                                                                      1-섹터 사이트 추가
    Vector v(500, 3000, nodeHeight);
    positionAlloc->Add(v);
    enbPosition.push_back(v);
    mobility.Install(ueNodes.at(plantedEnb));                                                   // UE 노드 설치
    plantedEnb++;

    // Add the 3-sector site                                                                    3-섹터 사이트 추가
    for (uint32_t index = 0; index < 3; index++, plantedEnb++)
    {
        Vector v(500, 2000, nodeHeight);
        positionAlloc->Add(v);
        enbPosition.push_back(v);
        mobility.Install(ueNodes.at(plantedEnb));                                               // UE 노드 설치
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);

    // Position of UEs attached to eNB                                                          eNB에 연결된 UE의 위치 설정
    for (uint32_t i = 0; i < nEnb; i++)
    {
        Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable>();
        posX->SetAttribute("Min", DoubleValue(enbPosition.at(i).x - roomLength * 0));
        posX->SetAttribute("Max", DoubleValue(enbPosition.at(i).x + roomLength * 0));
        Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable>();
        posY->SetAttribute("Min", DoubleValue(enbPosition.at(i).y - roomLength * 0));
        posY->SetAttribute("Max", DoubleValue(enbPosition.at(i).y + roomLength * 0));
        positionAlloc = CreateObject<ListPositionAllocator>();
        for (uint32_t j = 0; j < nUe; j++)
        {
            if (i == nEnb - 3)
            {
                positionAlloc->Add(
                    Vector(enbPosition.at(i).x + 10, enbPosition.at(i).y, nodeHeight));
            }
            else if (i == nEnb - 2)
            {
                positionAlloc->Add(Vector(enbPosition.at(i).x - std::sqrt(10),
                                          enbPosition.at(i).y + std::sqrt(10),
                                          nodeHeight));
            }
            else if (i == nEnb - 1)
            {
                positionAlloc->Add(Vector(enbPosition.at(i).x - std::sqrt(10),
                                          enbPosition.at(i).y - std::sqrt(10),
                                          nodeHeight));
            }
            else
            {
                positionAlloc->Add(Vector(posX->GetValue(), posY->GetValue(), nodeHeight));
            }
            mobility.SetPositionAllocator(positionAlloc);
        }
        mobility.Install(ueNodes.at(i));                                                        // UE 노드 설치
        BuildingsHelper::Install(ueNodes.at(i));                                                // 건물 헬퍼 설치
    }

    // Create Devices and install them in the Nodes (eNB and UE)                                장치 생성 및 노드에 설치 (eNB 및 UE)
    NetDeviceContainer enbDevs;
    std::vector<NetDeviceContainer> ueDevs;

    // power setting in dBm for small cells                                                     소형 셀의 전송 전력 설정 (dBM 단위)
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(20.0));
    enbDevs = lteHelper->InstallEnbDevice(oneSectorNodes);

    // power setting for three-sector macrocell                                                 3-섹터 매크로셀의 전송 전력 설정
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(43.0));

    // Beam width is made quite narrow so sectors can be noticed in the REM                     빔 폭을 좁혀서 REM에서 섹터를 확인할 수 있도록 설정
    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(0));
    lteHelper->SetEnbAntennaModelAttribute("HorizontalBeamwidth", DoubleValue(100));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(0.0));
    enbDevs.Add(lteHelper->InstallEnbDevice(threeSectorNodes.Get(0)));

    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(360 / 3));
    lteHelper->SetEnbAntennaModelAttribute("HorizontalBeamwidth", DoubleValue(100));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(0.0));
    enbDevs.Add(lteHelper->InstallEnbDevice(threeSectorNodes.Get(1)));

    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(2 * 360 / 3));
    lteHelper->SetEnbAntennaModelAttribute("HorizontalBeamwidth", DoubleValue(100));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(0.0));
    enbDevs.Add(lteHelper->InstallEnbDevice(threeSectorNodes.Get(2)));

    for (uint32_t i = 0; i < nEnb; i++)
    {
        NetDeviceContainer ueDev = lteHelper->InstallUeDevice(ueNodes.at(i));
        ueDevs.push_back(ueDev);
        lteHelper->Attach(ueDev, enbDevs.Get(i));
        EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
        EpsBearer bearer(q);
        lteHelper->ActivateDataRadioBearer(ueDev, bearer);                                      // 데이터 라디오 베어러 활성화
    }

    // by default, simulation will anyway stop right after the REM has been generated           REM 생성 후 시뮬레이션 종료 설정
    Simulator::Stop(Seconds(0.0069));

    Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper>();
    remHelper->SetAttribute("ChannelPath", StringValue("/ChannelList/0"));
    remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
    remHelper->SetAttribute("XMin", DoubleValue(-2000.0));
    remHelper->SetAttribute("XMax", DoubleValue(+2000.0));
    remHelper->SetAttribute("YMin", DoubleValue(-500.0));
    remHelper->SetAttribute("YMax", DoubleValue(+3500.0));
    remHelper->SetAttribute("Z", DoubleValue(1.5));
    remHelper->Install();                                                                       // REM 헬퍼 설치

    Simulator::Run();                                                                           // 시뮬레이션 실행

    //  GtkConfigStore config;
    //  config.ConfigureAttributes ();

    lteHelper = nullptr;
    Simulator::Destroy();
    return 0;
}
