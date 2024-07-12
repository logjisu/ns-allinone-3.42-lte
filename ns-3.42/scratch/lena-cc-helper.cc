/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#include "ns3/cc-helper.h"
#include "ns3/component-carrier.h"
#include "ns3/core-module.h"
#include <ns3/buildings-helper.h>
#include "ns3/config-store.h"

using namespace ns3;

void Print(ComponentCarrier cc);                                                                        // ComponenetCarrier 객체를 출력하는 함수 선언

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);                                                                          // 명령줄 인자를 처리하기 위한 CommandLine 객체 생성
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::ComponentCarrier::UlBandwidth", UintegerValue(100));                       // 컴포넌트 캐리어의 기본 설정 지정
    Config::SetDefault("ns3::ComponentCarrier::PrimaryCarrier", BooleanValue(true));

    // Parse again so you can override default values from the command line                             명령줄 인자를 재파싱하여 기본값을 덮어쓸 수 있게 함
    cmd.Parse(argc, argv);

    Ptr<CcHelper> cch = CreateObject<CcHelper>();                                                       // CcHelper 객체 생성 및 컴포넌트 캐리어 개수 설정
    cch->SetNumberOfComponentCarriers(2);

    std::map<uint8_t, ComponentCarrier> ccm = cch->EquallySpacedCcs();                                  // 동일 간격의 컴포넌트 캐리어 맵 생성

    std::cout << " CcMap size " << ccm.size() << std::endl;                                             // 맵의 크기 출력
    for (auto it = ccm.begin(); it != ccm.end(); it++)                                                  // 맵에 있는 각 컴포넌트 캐리어를 출력
    {
        Print(it->second);
    }

    Simulator::Stop(Seconds(1.05));                                                                     // 시뮬레이터 종료 시간 설정

    Simulator::Run();                                                                                   // 시뮬레이터 실행

    //GtkConfigStore config;
    //config.ConfigureAttributes ();

    Simulator::Destroy();                                                                               // 시뮬레이터 종료
    return 0;
}

void                                                                                                    // ComponentCarrier 객체의 속성을 출력하는 함수 정의
Print(ComponentCarrier cc)
{
    std::cout << " UlBandwidth " << uint16_t(cc.GetUlBandwidth()) << " DlBandwidth "
              << uint16_t(cc.GetDlBandwidth()) << " Dl Earfcn " << cc.GetDlEarfcn() << " Ul Earfcn "
              << cc.GetUlEarfcn() << " - Is this the Primary Channel? " << cc.IsPrimary()
              << std::endl;
}
