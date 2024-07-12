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
#include <ns3/buildings-module.h>

#include <iomanip>
#include <string>
#include <vector>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    uint32_t nEnbPerFloor = 1;                                                                  // 층당 eNB 수
    uint32_t nUe = 1;                                                                           // UE 수
    uint32_t nFloors = 0;                                                                       // 층 수, Friis 전파 모델의 경우 0
    double simTime = 1.0;                                                                       // 시뮬레이션 총 시간(초 단위)
    CommandLine cmd(__FILE__);

    cmd.AddValue("nEnb", "Number of eNodeBs per floor", nEnbPerFloor);                          // 층당 eNodeB 수
    cmd.AddValue("nUe", "Number of UEs", nUe);                                                  // UE 수
    cmd.AddValue("nFloors", "Number of floors, 0 for Friis propagation model", nFloors);        // 층 수, Friis 전파 모델의 경우 0
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);          // 시뮬레이션 총 시간 (초 단위)
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line                     명령줄에서 기본값을 재설정할 수 있도록 다시 파싱합니다.
    cmd.Parse(argc, argv);

    // Geometry of the scenario (in meters)                                                     시나리오의 기하학적 설정(미터 단위)
    // Assume squared building                                                                  정사각형 건물을 가정합니다
    double nodeHeight = 1.5;                                                                    // 노드 높이
    double roomHeight = 3;                                                                      // 방 높이
    double roomLength = 8;                                                                      // 방 길이
    uint32_t nRooms = std::ceil(std::sqrt(nEnbPerFloor));                                       // 방 수 (층당 eNB 수의 제곱근)
    uint32_t nEnb;

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    // lteHelper->EnableLogComponents ();
    // LogComponentEnable ("BuildingsPropagationLossModel", LOG_LEVEL_ALL);
    if (nFloors == 0)
    {
        lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));//Friis 전파 모델 설정
        nEnb = nEnbPerFloor;
    }
    else
    {
        lteHelper->SetAttribute("PathlossModel",
                                StringValue("ns3::HybridBuildingsPropagationLossModel"));       // 하이브리드BuildingsPropagationLoss모델 설정
        nEnb = nFloors * nEnbPerFloor;
    }

    // Create Nodes: eNodeB and UE                                                              노드 생성: eNB와 UE
    NodeContainer enbNodes;                                                                     // eNB 노드 컨테이너
    std::vector<NodeContainer> ueNodes;                                                         // UE 노드 컨테이너 배열

    enbNodes.Create(nEnb);                                                                      // eNB 수 만큼 생성
    for (uint32_t i = 0; i < nEnb; i++)
    {
        NodeContainer ueNode;
        ueNode.Create(nUe);                                                                     // 각 eNB에 연결될 UE 수 만큼 생성
        ueNodes.push_back(ueNode);
    }

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    std::vector<Vector> enbPosition;                                                            // eNB의 위치 벡터
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    Ptr<Building> building;

    if (nFloors == 0)
    {
        // Position of eNBs                                                                     eNB의 위치 설정
        uint32_t plantedEnb = 0;
        for (uint32_t row = 0; row < nRooms; row++)
        {
            for (uint32_t column = 0; column < nRooms && plantedEnb < nEnbPerFloor;
                 column++, plantedEnb++)
            {
                Vector v(roomLength * (column + 0.5), roomLength * (row + 0.5), nodeHeight);
                positionAlloc->Add(v);
                enbPosition.push_back(v);
                mobility.Install(ueNodes.at(plantedEnb));                                       // eNB에 연결된 UE의 위치 설정
            }
        }
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(enbNodes);                                                             // eNB에 위치 할당
        BuildingsHelper::Install(enbNodes);                                                     // 건물 모듈을 eNB에 설치

        // Position of UEs attached to eNB                                                      eNB에 연결된 UE의 위치 설정
        for (uint32_t i = 0; i < nEnb; i++)
        {
            Ptr<UniformRandomVariable> posX = CreateObject<UniformRandomVariable>();
            posX->SetAttribute("Min", DoubleValue(enbPosition.at(i).x - roomLength * 0.5));
            posX->SetAttribute("Max", DoubleValue(enbPosition.at(i).x + roomLength * 0.5));
            Ptr<UniformRandomVariable> posY = CreateObject<UniformRandomVariable>();
            posY->SetAttribute("Min", DoubleValue(enbPosition.at(i).y - roomLength * 0.5));
            posY->SetAttribute("Max", DoubleValue(enbPosition.at(i).y + roomLength * 0.5));
            positionAlloc = CreateObject<ListPositionAllocator>();
            for (uint32_t j = 0; j < nUe; j++)
            {
                positionAlloc->Add(Vector(posX->GetValue(), posY->GetValue(), nodeHeight));
                mobility.SetPositionAllocator(positionAlloc);
            }
            mobility.Install(ueNodes.at(i));                                                    // UE에 위치 할당
            BuildingsHelper::Install(ueNodes.at(i));                                            // 건물 모듈을 UE에 설치
        }
    }
    else
    {
        building = CreateObject<Building>();                                                    // 다층 건물 설정
        building->SetBoundaries(
            Box(0.0, nRooms * roomLength, 0.0, nRooms * roomLength, 0.0, nFloors * roomHeight));
        building->SetBuildingType(Building::Residential);
        building->SetExtWallsType(Building::ConcreteWithWindows);
        building->SetNFloors(nFloors);
        building->SetNRoomsX(nRooms);
        building->SetNRoomsY(nRooms);
        mobility.Install(enbNodes);                                                             // eNB에 위치 할당
        BuildingsHelper::Install(enbNodes);                                                     // 건물 모듈을 eNB에 설치
        uint32_t plantedEnb = 0;
        for (uint32_t floor = 0; floor < nFloors; floor++)
        {
            uint32_t plantedEnbPerFloor = 0;
            for (uint32_t row = 0; row < nRooms; row++)
            {
                for (uint32_t column = 0; column < nRooms && plantedEnbPerFloor < nEnbPerFloor;
                     column++, plantedEnb++, plantedEnbPerFloor++)
                {
                    Vector v(roomLength * (column + 0.5),
                             roomLength * (row + 0.5),
                             nodeHeight + roomHeight * floor);
                    positionAlloc->Add(v);
                    enbPosition.push_back(v);
                    Ptr<MobilityModel> mmEnb = enbNodes.Get(plantedEnb)->GetObject<MobilityModel>();
                    mmEnb->SetPosition(v);

                    // Positioning UEs attached to eNB                                          eNB에 연결된 UE의 위치 설정
                    mobility.Install(ueNodes.at(plantedEnb));
                    BuildingsHelper::Install(ueNodes.at(plantedEnb));                           // 건물 모듈을 UE에 설치
                    for (uint32_t ue = 0; ue < nUe; ue++)
                    {
                        Ptr<MobilityModel> mmUe =
                            ueNodes.at(plantedEnb).Get(ue)->GetObject<MobilityModel>();
                        Vector vUe(v.x, v.y, v.z);
                        mmUe->SetPosition(vUe);
                    }
                }
            }
        }
    }

    // Create Devices and install them in the Nodes (eNB and UE)                                장치 생성 및 노드에 설치(eNB와 UE)
    NetDeviceContainer enbDevs;                                                                 // eNB 장치 컨테이너
    std::vector<NetDeviceContainer> ueDevs;                                                     // UE 장치 컨테이너 배열
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);                                            // eNB 장치 설치
    for (uint32_t i = 0; i < nEnb; i++)
    {
        NetDeviceContainer ueDev = lteHelper->InstallUeDevice(ueNodes.at(i));                   // UE 장치 설치
        ueDevs.push_back(ueDev);
        lteHelper->Attach(ueDev, enbDevs.Get(i));                                               // UE를 eNB에 연결
        EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
        EpsBearer bearer(q);
        lteHelper->ActivateDataRadioBearer(ueDev, bearer);                                      // 데이터 라디오 베어러 활성화
    }

    Simulator::Stop(Seconds(simTime));
    lteHelper->EnableTraces();                                                                  // 트레이스 활성화

    Simulator::Run();                                                                           // 시뮬레이션 실행

    /*GtkConfigStore config;
    config.ConfigureAttributes ();*/

    Simulator::Destroy();                                                                       // 시뮬레이션 종료
    return 0;
}
