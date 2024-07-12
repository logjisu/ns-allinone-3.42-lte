/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include <ns3/applications-module.h>
#include <ns3/buildings-module.h>
#include <ns3/config-store-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/log.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-helper.h>

#include <iomanip>
#include <ios>
#include <string>
#include <vector>

// The topology of this simulation program is inspired from                                             이 시뮬레이션 프로그램의 토폴로지(위상)은 3GPP R4-092042,
// 3GPP R4-092042, Section 4.2.1 Dual Stripe Model                                                      Section 4.2.1 Dual Stripe Model에서 영감을 받았습니다.
// note that the term "apartments" used in that document matches with                                   주의: 해당 만서에서 "apartments"라는 용어는 'BuildingsMobilityModel'
// the term "room" used in the BuildingsMobilityModel                                                   에서 사용하는 'room'과 일치합니다.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaDualStripe");                                                              // 로그 컴포넌트 정의

/**
 * Check if two boxes are overlapping.                                                                  두 박스가 겹치는지 확인
 *
 * \param a First box.                                                                                  첫 번째 박스
 * \param b Second box.                                                                                 두 번째 박스
 * \return true if the boxes are overlapping, false otherwise.                                          박스가 겹치면 참 아니면 거짓
 */
bool
AreOverlapping(Box a, Box b)
{
    return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}

/**
 * Class that takes care of installing blocks of the                                                    주어진 영역에 빌딩 블록을 설치하는 클래스 정의
 * buildings in a given area. Buildings are installed in pairs                                          빌딩은 듀얼 스트라이프 시나리오에서와 같이 쌍으로 설치됨
 * as in dual stripe scenario.
 */
class FemtocellBlockAllocator
{
  public:
    /**
     * Constructor                                                                                      생성자
     * \param area the total area                                                                       전체 영역
     * \param nApartmentsX the number of apartments in the X direction                                  X방향의 아파트 수
     * \param nFloors the number of floors                                                              층 수
     */
    FemtocellBlockAllocator(Box area, uint32_t nApartmentsX, uint32_t nFloors);
    /**
     * Function that creates building blocks.                                                           빌딩 블록을 생성하는 함수
     * \param n the number of blocks to create                                                          생성할 블록 수
     */
    void Create(uint32_t n);
    /// Create function                                                                                 기본 생성 함수
    void Create();

  private:
    /**
     * Function that checks if the box area is overlapping with some of previously created building     박스 영역이 이전에 생성된 빌딩 블록과 겹치는지 확인하는 함수
     * blocks.
     * \param box the area to check                                                                     확인할 영역
     * \returns true if there is an overlap                                                             겹치면 참
     */
    bool OverlapsWithAnyPrevious(Box box);
    Box m_area;                           ///< Area                                                     영역
    uint32_t m_nApartmentsX;              ///< X apartments                                             X방향의 아파트 수
    uint32_t m_nFloors;                   ///< number of floors                                         층 수
    std::list<Box> m_previousBlocks;      ///< previous bocks                                           이전 블록들
    double m_xSize;                       ///< X size                                                   X크기
    double m_ySize;                       ///< Y size                                                   Y크기
    Ptr<UniformRandomVariable> m_xMinVar; ///< X minimum variance                                       X 최소 값
    Ptr<UniformRandomVariable> m_yMinVar; ///< Y minimum variance                                       Y 최소 값
};

FemtocellBlockAllocator::FemtocellBlockAllocator(Box area, uint32_t nApartmentsX, uint32_t nFloors)
    : m_area(area),
      m_nApartmentsX(nApartmentsX),
      m_nFloors(nFloors),
      m_xSize(nApartmentsX * 10 + 20),
      m_ySize(70)
{
    m_xMinVar = CreateObject<UniformRandomVariable>();
    m_xMinVar->SetAttribute("Min", DoubleValue(area.xMin));
    m_xMinVar->SetAttribute("Max", DoubleValue(area.xMax - m_xSize));
    m_yMinVar = CreateObject<UniformRandomVariable>();
    m_yMinVar->SetAttribute("Min", DoubleValue(area.yMin));
    m_yMinVar->SetAttribute("Max", DoubleValue(area.yMax - m_ySize));
}

void
FemtocellBlockAllocator::Create(uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
    {
        Create();
    }
}

void
FemtocellBlockAllocator::Create()
{
    Box box;
    uint32_t attempt = 0;
    do
    {
        NS_ASSERT_MSG(attempt < 100,
                      "Too many failed attempts to position apartment block. Too many blocks? Too "     // 아파트 블록을 배치하려는 시도가 너무 많습니다. 블록이 너무 많거나?
                      "small area?");                                                                   // 영역이 너무 작습니까?
        box.xMin = m_xMinVar->GetValue();
        box.xMax = box.xMin + m_xSize;
        box.yMin = m_yMinVar->GetValue();
        box.yMax = box.yMin + m_ySize;
        ++attempt;
    } while (OverlapsWithAnyPrevious(box));

    NS_LOG_LOGIC("allocated non overlapping block " << box);                                            // 겹치지 않는 블록을 할당했습니다.
    m_previousBlocks.push_back(box);
    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator>();
    gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(1));
    gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(10 * m_nApartmentsX));
    gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(10 * 2));
    gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(10));
    gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(10));
    gridBuildingAllocator->SetAttribute("Height", DoubleValue(3 * m_nFloors));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(m_nApartmentsX));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(2));
    gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(m_nFloors));
    gridBuildingAllocator->SetAttribute("MinX", DoubleValue(box.xMin + 10));
    gridBuildingAllocator->SetAttribute("MinY", DoubleValue(box.yMin + 10));
    gridBuildingAllocator->Create(2);
}

bool
FemtocellBlockAllocator::OverlapsWithAnyPrevious(Box box)
{
    for (auto it = m_previousBlocks.begin(); it != m_previousBlocks.end(); ++it)
    {
        if (AreOverlapping(*it, box))
        {
            return true;
        }
    }
    return false;
}

/**
 * Print a list of buildings that can be plotted using Gnuplot.                                         Gunplot을 사용하여 플롯할 수 있는 빌딩 목록을 파일로 출력
 *
 * \param filename the output file name.                                                                출력 파일 이름
 */
void
PrintGnuplottableBuildingListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                                   // 파일을 열 수 없습니다
        return;
    }
    uint32_t index = 0;
    for (auto it = BuildingList::Begin(); it != BuildingList::End(); ++it)
    {
        ++index;
        Box box = (*it)->GetBoundaries();
        outFile << "set object " << index << " rect from " << box.xMin << "," << box.yMin << " to "
                << box.xMax << "," << box.yMax << " front fs empty " << std::endl;
    }
}

/**
 * Print a list of UEs that can be plotted using Gnuplot.                                               Gunplot을 사용하여 플롯할 수 있는 ENB 목록을 파일로 출력합니다.
 *
 * \param filename the output file name.                                                                출력 파일 이름
 */
void
PrintGnuplottableUeListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                                   // 파일을 열 수 없습니다
        return;
    }
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteUeNetDevice> uedev = node->GetDevice(j)->GetObject<LteUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << uedev->GetImsi() << "\" at " << pos.x << "," << pos.y
                        << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps "
                           "0.3 lc rgb \"grey\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

/**
 * Print a list of ENBs that can be plotted using Gnuplot.                                              Gnuplot을 사용하여 플로팅할 수 있는 ENB 목록을 인쇄합니다.
 *
 * \param filename the output file name.                                                                출력 파일 이름
 */
void
PrintGnuplottableEnbListToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename, std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);                                                   // 파일을 열 수 없습니다.
        return;
    }
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteEnbNetDevice> enbdev = node->GetDevice(j)->GetObject<LteEnbNetDevice>();
            if (enbdev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << "set label \"" << enbdev->GetCellId() << "\" at " << pos.x << ","
                        << pos.y
                        << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 "
                           "ps 0.3 lc rgb \"white\" offset 0,0"
                        << std::endl;
            }
        }
    }
}

/// Number of femtocell blocks                                                                          페모셀 블록 수
static ns3::GlobalValue g_nBlocks("nBlocks",
                                  "Number of femtocell blocks",
                                  ns3::UintegerValue(1),
                                  ns3::MakeUintegerChecker<uint32_t>());

/// Number of apartments along the X axis in a femtocell block                                          페모셀 블록의 X축 아파트 수
static ns3::GlobalValue g_nApartmentsX("nApartmentsX",
                                       "Number of apartments along the X axis in a femtocell block",
                                       ns3::UintegerValue(10),
                                       ns3::MakeUintegerChecker<uint32_t>());

/// Number of floors                                                                                    층 수
static ns3::GlobalValue g_nFloors("nFloors",
                                  "Number of floors",
                                  ns3::UintegerValue(1),
                                  ns3::MakeUintegerChecker<uint32_t>());

/// How many macro sites there are                                                                      매크로 사이트 수
static ns3::GlobalValue g_nMacroEnbSites("nMacroEnbSites",
                                         "How many macro sites there are",
                                         ns3::UintegerValue(3),
                                         ns3::MakeUintegerChecker<uint32_t>());

/// (minimum) number of sites along the X-axis of the hex grid                                          육각형 그리드 X축에서의 최소 사이트 수
static ns3::GlobalValue g_nMacroEnbSitesX(
    "nMacroEnbSitesX",
    "(minimum) number of sites along the X-axis of the hex grid",
    ns3::UintegerValue(1),
    ns3::MakeUintegerChecker<uint32_t>());

/// min distance between two nearby macro cell sites                                                    인근 매크로 셀 사이의 최소 거리
static ns3::GlobalValue g_interSiteDistance("interSiteDistance",
                                            "min distance between two nearby macro cell sites",
                                            ns3::DoubleValue(500),
                                            ns3::MakeDoubleChecker<double>());

/// how much the UE area extends outside the macrocell grid, expressed as fraction of the               매크로 셀 그리드 외부로 UE 영역이 얼마나 확장되는지,
/// interSiteDistance                                                                                   interSiteDistance의 비율로 표현
static ns3::GlobalValue g_areaMarginFactor(
    "areaMarginFactor",
    "how much the UE area extends outside the macrocell grid, "
    "expressed as fraction of the interSiteDistance",
    ns3::DoubleValue(0.5),
    ns3::MakeDoubleChecker<double>());

/// How many macrocell UEs there are per square meter                                                   매크로 셀 UEs 당 정밀도
static ns3::GlobalValue g_macroUeDensity("macroUeDensity",
                                         "How many macrocell UEs there are per square meter",
                                         ns3::DoubleValue(0.00002),
                                         ns3::MakeDoubleChecker<double>());

/// The HeNB deployment ratio as per 3GPP R4-092042                                                     3GPP R4-092042에 따른 HeNB 배치 비율
static ns3::GlobalValue g_homeEnbDeploymentRatio("homeEnbDeploymentRatio",
                                                 "The HeNB deployment ratio as per 3GPP R4-092042",
                                                 ns3::DoubleValue(0.2),
                                                 ns3::MakeDoubleChecker<double>());

/// The HeNB activation ratio as per 3GPP R4-092042                                                     3GPP R4-092042에 따른 HeNB 활성화 비율
static ns3::GlobalValue g_homeEnbActivationRatio("homeEnbActivationRatio",
                                                 "The HeNB activation ratio as per 3GPP R4-092042",
                                                 ns3::DoubleValue(0.5),
                                                 ns3::MakeDoubleChecker<double>());

/// How many (on average) home UEs per HeNB there are in the simulation                                 시뮬레이션에서 HeNB 당 평균 집 UEs 수
static ns3::GlobalValue g_homeUesHomeEnbRatio(
    "homeUesHomeEnbRatio",
    "How many (on average) home UEs per HeNB there are in the simulation",
    ns3::DoubleValue(1.0),
    ns3::MakeDoubleChecker<double>());

/// TX power [dBm] used by macro eNBs                                                                   매그로 eNB가 사용하는 TX 파워 [dBM]
static ns3::GlobalValue g_macroEnbTxPowerDbm("macroEnbTxPowerDbm",
                                             "TX power [dBm] used by macro eNBs",
                                             ns3::DoubleValue(46.0),
                                             ns3::MakeDoubleChecker<double>());

/// TX power [dBm] used by HeNBs                                                                        HeNB가 사용하는 TX 파워 [dBM]
static ns3::GlobalValue g_homeEnbTxPowerDbm("homeEnbTxPowerDbm",
                                            "TX power [dBm] used by HeNBs",
                                            ns3::DoubleValue(20.0),
                                            ns3::MakeDoubleChecker<double>());

/// DL EARFCN used by macro eNBs                                                                        매크로 eNB가 사용하는 DL EARFCN
static ns3::GlobalValue g_macroEnbDlEarfcn("macroEnbDlEarfcn",
                                           "DL EARFCN used by macro eNBs",
                                           ns3::UintegerValue(100),
                                           ns3::MakeUintegerChecker<uint16_t>());

/// DL EARFCN used by HeNBs                                                                             HeNB가 사용하는 DL EARFCN
static ns3::GlobalValue g_homeEnbDlEarfcn("homeEnbDlEarfcn",
                                          "DL EARFCN used by HeNBs",
                                          ns3::UintegerValue(100),
                                          ns3::MakeUintegerChecker<uint16_t>());

/// Bandwidth [num RBs] used by macro eNBs                                                              매크로 eNB가 사용하는 대역폭 [num RBs]
static ns3::GlobalValue g_macroEnbBandwidth("macroEnbBandwidth",
                                            "bandwidth [num RBs] used by macro eNBs",
                                            ns3::UintegerValue(25),
                                            ns3::MakeUintegerChecker<uint16_t>());

/// Bandwidth [num RBs] used by HeNBs                                                                   HeNB가 사용하는 대역폭 [num RBs]
static ns3::GlobalValue g_homeEnbBandwidth("homeEnbBandwidth",
                                           "bandwidth [num RBs] used by HeNBs",
                                           ns3::UintegerValue(25),
                                           ns3::MakeUintegerChecker<uint16_t>());

/// Total duration of the simulation [s]                                                                시뮬레이션 총 기간 [초]
static ns3::GlobalValue g_simTime("simTime",
                                  "Total duration of the simulation [s]",
                                  ns3::DoubleValue(0.25),
                                  ns3::MakeDoubleChecker<double>());

/// If true, will generate a REM and then abort the simulation                                          참인 경우 REM을 생성하고 시뮬레이션을 중단합니다.
static ns3::GlobalValue g_generateRem(
    "generateRem",
    "if true, will generate a REM and then abort the simulation;"
    "if false, will run the simulation normally (without generating any REM)",                          // 거짓인 경우 일반적으로 시뮬레이션을 실행합니다(REM 생성 없음)
    ns3::BooleanValue(false),
    ns3::MakeBooleanChecker());

/// Resource Block Id of Data Channel, for which REM will be generated.                                 데이터 채널의 리소스 블록 ID, REM이 생성됩니다.
static ns3::GlobalValue g_remRbId(
    "remRbId",
    "Resource Block Id of Data Channel, for which REM will be generated;"
    "default value is -1, what means REM will be averaged from all RBs of "                             // 기본값은 -1입니다. 이는 REM이 제어 채널의 모든 RB에서
    "Control Channel",                                                                                  // 평균화됨을 의미합니다.
    ns3::IntegerValue(-1),
    MakeIntegerChecker<int32_t>());

/// If true, will setup the EPC to simulate an end-to-end topology.                                     참인 경우 EPC를 설정하여 종단 간 토폴로지를 시뮬레이트합니다.
static ns3::GlobalValue g_epc(
    "epc",
    "If true, will setup the EPC to simulate an end-to-end topology, "
    "with real IP applications over PDCP and RLC UM (or RLC AM by changing "                            // PDCP 및 RLC UM을 사용하여 실제 IP 애플리케이션을 시뮬레이트합니다.
    "the default value of EpsBearerToRlcMapping e.g. to RLC_AM_ALWAYS). "                               // 거짓인 경우 LTE 무선 접속만 시뮬레이트합니다. (RLC SM 사용)
    "If false, only the LTE radio access will be simulated with RLC SM.",
    ns3::BooleanValue(false),
    ns3::MakeBooleanChecker());

/// if true, will activate data flows in the downlink when EPC is being used.                           참인 경우 EPC를 사용할 때 다운링크 데이터 흐름을 활성화합니다.
static ns3::GlobalValue g_epcDl(
    "epcDl",
    "if true, will activate data flows in the downlink when EPC is being used. "
    "If false, downlink flows won't be activated. "                                                     // 거짓인 경우 다운링크 흐름이 활성화되지 않습니다.
    "If EPC is not used, this parameter will be ignored.",                                              // EPC를 사용하지 않을 경우 이 매개변수는 무시됩니다.
    ns3::BooleanValue(true),
    ns3::MakeBooleanChecker());

/// if true, will activate data flows in the uplink when EPC is being used.                             참인 경우 EPC를 사용할 때 업링크 데이터 흐름을 활성화합니다.
static ns3::GlobalValue g_epcUl(
    "epcUl",
    "if true, will activate data flows in the uplink when EPC is being used. "
    "If false, uplink flows won't be activated. "                                                       // 거짓인 경우 업링크 흐름이 활성화되지 않습니다.
    "If EPC is not used, this parameter will be ignored.",                                              // EPC를 사용하지 않을 경우 이 매개변수는 무시됩니다.
    ns3::BooleanValue(true),
    ns3::MakeBooleanChecker());

/// if true, the UdpClient application will be used.                                                    참인 경우 UdpClient 애플리케이션을 사용합니다.
static ns3::GlobalValue g_useUdp(
    "useUdp",
    "if true, the UdpClient application will be used. "
    "Otherwise, the BulkSend application will be used over a TCP connection. "                          // 거짓일 경우 TCP 연결을 통해 BulkSend 애플리케이션을 사용합니다.
    "If EPC is not used, this parameter will be ignored.",                                              // EPC를 사용하지 않을 경우 이 매개변수는 무시됩니다.
    ns3::BooleanValue(true),
    ns3::MakeBooleanChecker());

/// The path of the fading trace (by default no fading trace is loaded, i.e., fading is not             페이딩 추적의 경로(기본적으로 페이딩 추적이 없음, 즉 페이딩이
/// considered)                                                                                         고려되지 않음)
static ns3::GlobalValue g_fadingTrace("fadingTrace",
                                      "The path of the fading trace (by default no fading trace "
                                      "is loaded, i.e., fading is not considered)",
                                      ns3::StringValue(""),
                                      ns3::MakeStringChecker());

/// How many bearers per UE there are in the simulation                                                 시뮬레이션에서 UE 당 베어러 수
static ns3::GlobalValue g_numBearersPerUe("numBearersPerUe",
                                          "How many bearers per UE there are in the simulation",
                                          ns3::UintegerValue(1),
                                          ns3::MakeUintegerChecker<uint16_t>());

/// SRS Periodicity (has to be at least greater than the number of UEs per eNB)                         SRS 주기성 (eNB 당 UE 수 보다 적어야 함)
static ns3::GlobalValue g_srsPeriodicity("srsPeriodicity",
                                         "SRS Periodicity (has to be at least "
                                         "greater than the number of UEs per eNB)",
                                         ns3::UintegerValue(80),
                                         ns3::MakeUintegerChecker<uint16_t>());

/// Minimum speed value of macro UE with random waypoint model [m/s].                                   무작위 웨이포인트 모델의 매크로 UE의 최소 속도 [m/s]
static ns3::GlobalValue g_outdoorUeMinSpeed(
    "outdoorUeMinSpeed",
    "Minimum speed value of macro UE with random waypoint model [m/s].",
    ns3::DoubleValue(0.0),
    ns3::MakeDoubleChecker<double>());

/// Maximum speed value of macro UE with random waypoint model [m/s].                                   무작위 웨이포인트 모델의 매크로 UE의 최대 속도 [m/s]
static ns3::GlobalValue g_outdoorUeMaxSpeed(
    "outdoorUeMaxSpeed",
    "Maximum speed value of macro UE with random waypoint model [m/s].",
    ns3::DoubleValue(0.0),
    ns3::MakeDoubleChecker<double>());

int
main(int argc, char* argv[])
{
    // change some default attributes so that they are reasonable for                                   이 시나리오에 합리적인 설정을 위해 일부 기본 속성을 변경합니다.
    // this scenario, but do this before processing command line                                        명령 줄 인수를 처리하기 전에 이 작업을 수행하여 사용자가 이 설정을
    // arguments, so that the user is allowed to override these settings                                재정의할 수 있도록 합니다.
    Config::SetDefault("ns3::UdpClient::Interval", TimeValue(MilliSeconds(1)));
    Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(10 * 1024));

    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();
    // parse again so you can override input file default values via command line                       명령 줄을 통해 입력 파일의 기본값을 재정의할 수 있도록 다시 구문 분석합니다.
    cmd.Parse(argc, argv);

    // the scenario parameters get their values from the global attributes defined above                시나리오 매개변수는 위에서 정의된 전역 속성에서 값을 가져옵니다.
    UintegerValue uintegerValue;
    IntegerValue integerValue;
    DoubleValue doubleValue;
    BooleanValue booleanValue;
    StringValue stringValue;
    GlobalValue::GetValueByName("nBlocks", uintegerValue);                                              // 블록 수
    uint32_t nBlocks = uintegerValue.Get();
    GlobalValue::GetValueByName("nApartmentsX", uintegerValue);                                         // 아파트 당 X축 수
    uint32_t nApartmentsX = uintegerValue.Get();
    GlobalValue::GetValueByName("nFloors", uintegerValue);                                              // 층 수
    uint32_t nFloors = uintegerValue.Get();
    GlobalValue::GetValueByName("nMacroEnbSites", uintegerValue);                                       // 매크로 eNB 사이트 수
    uint32_t nMacroEnbSites = uintegerValue.Get();
    GlobalValue::GetValueByName("nMacroEnbSitesX", uintegerValue);                                      // 매크로 eNB X축 사이트 수
    uint32_t nMacroEnbSitesX = uintegerValue.Get();
    GlobalValue::GetValueByName("interSiteDistance", doubleValue);                                      // 사이트 간 거리
    double interSiteDistance = doubleValue.Get();
    GlobalValue::GetValueByName("areaMarginFactor", doubleValue);                                       // 영역 여유 요인
    double areaMarginFactor = doubleValue.Get();
    GlobalValue::GetValueByName("macroUeDensity", doubleValue);                                         // 매크로 UE 밀도
    double macroUeDensity = doubleValue.Get();
    GlobalValue::GetValueByName("homeEnbDeploymentRatio", doubleValue);                                 // 홈 eNB 배치 비율
    double homeEnbDeploymentRatio = doubleValue.Get();
    GlobalValue::GetValueByName("homeEnbActivationRatio", doubleValue);                                 // 홈 eNB 활성화 비율
    double homeEnbActivationRatio = doubleValue.Get();
    GlobalValue::GetValueByName("homeUesHomeEnbRatio", doubleValue);                                    // 홈 UE 대 홈 eNB 비율
    double homeUesHomeEnbRatio = doubleValue.Get();
    GlobalValue::GetValueByName("macroEnbTxPowerDbm", doubleValue);                                     // 매크로 eNB 전송 전력 (dBm)
    double macroEnbTxPowerDbm = doubleValue.Get();
    GlobalValue::GetValueByName("homeEnbTxPowerDbm", doubleValue);                                      // 홈 eNB 전송 전력 (dBm)
    double homeEnbTxPowerDbm = doubleValue.Get();
    GlobalValue::GetValueByName("macroEnbDlEarfcn", uintegerValue);                                     // 매크로 eNB DL EARFCN
    uint32_t macroEnbDlEarfcn = uintegerValue.Get();
    GlobalValue::GetValueByName("homeEnbDlEarfcn", uintegerValue);                                      // 홈 eNB DL EARFCN
    uint32_t homeEnbDlEarfcn = uintegerValue.Get();
    GlobalValue::GetValueByName("macroEnbBandwidth", uintegerValue);                                    // 매크로 eNB 대역폭
    uint16_t macroEnbBandwidth = uintegerValue.Get();
    GlobalValue::GetValueByName("homeEnbBandwidth", uintegerValue);                                     // 홈 eNB 대역폭
    uint16_t homeEnbBandwidth = uintegerValue.Get();
    GlobalValue::GetValueByName("simTime", doubleValue);                                                // 시뮬레이션 시간
    double simTime = doubleValue.Get();
    GlobalValue::GetValueByName("epc", booleanValue);                                                   // EPC 사용 여부
    bool epc = booleanValue.Get();
    GlobalValue::GetValueByName("epcDl", booleanValue);                                                 // EPC 사용 시 다운링크 데이터 흐름 활성화 여부
    bool epcDl = booleanValue.Get();
    GlobalValue::GetValueByName("epcUl", booleanValue);                                                 // EPC 사용 시 업링크 데이터 흐름 활성화 여부
    bool epcUl = booleanValue.Get();
    GlobalValue::GetValueByName("useUdp", booleanValue);                                                // UDP 클라이언트 애플리케이션 사용 여부
    bool useUdp = booleanValue.Get();
    GlobalValue::GetValueByName("generateRem", booleanValue);                                           // REM 생성 여부
    bool generateRem = booleanValue.Get();
    GlobalValue::GetValueByName("remRbId", integerValue);                                               // REM RB ID
    int32_t remRbId = integerValue.Get();
    GlobalValue::GetValueByName("fadingTrace", stringValue);                                            // 페이딩 추적 파일 이름
    std::string fadingTrace = stringValue.Get();
    GlobalValue::GetValueByName("numBearersPerUe", uintegerValue);                                      // UE 당 베어러 수
    uint16_t numBearersPerUe = uintegerValue.Get();
    GlobalValue::GetValueByName("srsPeriodicity", uintegerValue);                                       // SRS 주기성
    uint16_t srsPeriodicity = uintegerValue.Get();
    GlobalValue::GetValueByName("outdoorUeMinSpeed", doubleValue);                                      // 외부 UE 최소 속도
    uint16_t outdoorUeMinSpeed = doubleValue.Get();
    GlobalValue::GetValueByName("outdoorUeMaxSpeed", doubleValue);                                      // 외부 UE 최대 속도
    uint16_t outdoorUeMaxSpeed = doubleValue.Get();

    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(srsPeriodicity));                // LTE eNB RRC의 SRS 주기성 설정

    Box macroUeBox;                                                                                     // 매크로 UE 영역을 정의합니다.
    double ueZ = 1.5;
    if (nMacroEnbSites > 0)
    {
        uint32_t currentSite = nMacroEnbSites - 1;                                                      // 매크로 eNB 사이트가 있는 경우
        uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
        uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
        uint32_t rowIndex = biRowIndex * 2 + 1;
        if (biRowRemainder >= nMacroEnbSitesX)
        {
            ++rowIndex;
        }
        uint32_t nMacroEnbSitesY = rowIndex;
        NS_LOG_LOGIC("nMacroEnbSitesY = " << nMacroEnbSitesY);

        macroUeBox = Box(-areaMarginFactor * interSiteDistance,                                         // 매크로 UE 박스를 설정합니다.
                         (nMacroEnbSitesX + areaMarginFactor) * interSiteDistance,
                         -areaMarginFactor * interSiteDistance,
                         (nMacroEnbSitesY - 1) * interSiteDistance * sqrt(0.75) +
                             areaMarginFactor * interSiteDistance,
                         ueZ,
                         ueZ);
    }
    else
    {
        // still need the box to place femtocell blocks                                                 매크로 eNB 사이트가 없는 경우, 여전히 박스를 정의하여 페모셀 블록을 배치함
        macroUeBox = Box(0, 150, 0, 150, ueZ, ueZ);
    }

    FemtocellBlockAllocator blockAllocator(macroUeBox, nApartmentsX, nFloors);                          // 페모셀 블록 할당기를 사용하여 매크로 UE 영역에서 블록을 생성합니다.
    blockAllocator.Create(nBlocks);

    uint32_t nHomeEnbs = round(4 * nApartmentsX * nBlocks * nFloors * homeEnbDeploymentRatio *          // 홈 eNB 및 UE 수를 계산합니다.
                               homeEnbActivationRatio);
    NS_LOG_LOGIC("nHomeEnbs = " << nHomeEnbs);
    uint32_t nHomeUes = round(nHomeEnbs * homeUesHomeEnbRatio);
    NS_LOG_LOGIC("nHomeUes = " << nHomeUes);
    double macroUeAreaSize =                                                                            // 매크로 UE 영역 크기를 계산하여 매크로 UE 수를 설정합니다.
        (macroUeBox.xMax - macroUeBox.xMin) * (macroUeBox.yMax - macroUeBox.yMin);
    uint32_t nMacroUes = round(macroUeAreaSize * macroUeDensity);
    NS_LOG_LOGIC("nMacroUes = " << nMacroUes << " (density=" << macroUeDensity << ")");

    NodeContainer homeEnbs;                                                                             // 노드 컨테이너를 생성하여 각각의 UE 및 eNB에 할당합니다.
    homeEnbs.Create(nHomeEnbs);
    NodeContainer macroEnbs;
    macroEnbs.Create(3 * nMacroEnbSites);
    NodeContainer homeUes;
    homeUes.Create(nHomeUes);
    NodeContainer macroUes;
    macroUes.Create(nMacroUes);

    MobilityHelper mobility;                                                                            // 이동성Helper를 설정하고 이동성 모델을 "ConstantPositionMobilityModel"
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");                                    // 로 설정합니다.

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();                                               // LTEHelper를 생성하고 속성을 설정합니다.
    lteHelper->SetAttribute("PathlossModel",
                            StringValue("ns3::HybridBuildingsPropagationLossModel"));
    lteHelper->SetPathlossModelAttribute("ShadowSigmaExtWalls", DoubleValue(0));
    lteHelper->SetPathlossModelAttribute("ShadowSigmaOutdoor", DoubleValue(1));
    lteHelper->SetPathlossModelAttribute("ShadowSigmaIndoor", DoubleValue(1.5));
    // use always LOS model
    lteHelper->SetPathlossModelAttribute("Los2NlosThr", DoubleValue(1e6));
    lteHelper->SetSpectrumChannelType("ns3::MultiModelSpectrumChannel");

    //   lteHelper->EnableLogComponents ();
    //   LogComponentEnable ("PfFfMacScheduler", LOG_LEVEL_ALL);

    if (!fadingTrace.empty())                                                                           // 페이딩 모델이 지정된 경우 설정합니다.
    {
        lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));
        lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(fadingTrace));
    }

    Ptr<PointToPointEpcHelper> epcHelper;
    if (epc)
    {
        NS_LOG_LOGIC("enabling EPC");                                                                   // EPC 사용이 설정된 경우 EPC를 활성화합니다.
        epcHelper = CreateObject<PointToPointEpcHelper>();
        lteHelper->SetEpcHelper(epcHelper);
    }

    // Macro eNBs in 3-sector hex grid                                                                  6각 모양의 3섹터 안의 매크로 eNB

    mobility.Install(macroEnbs);                                                                        // 매크로 eNB들을 설치하고 위치 및 장치를 설정합니다.
    BuildingsHelper::Install(macroEnbs);
    Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper =
        CreateObject<LteHexGridEnbTopologyHelper>();
    lteHexGridEnbTopologyHelper->SetLteHelper(lteHelper);
    lteHexGridEnbTopologyHelper->SetAttribute("InterSiteDistance", DoubleValue(interSiteDistance));
    lteHexGridEnbTopologyHelper->SetAttribute("MinX", DoubleValue(interSiteDistance / 2));
    lteHexGridEnbTopologyHelper->SetAttribute("GridWidth", UintegerValue(nMacroEnbSitesX));
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(macroEnbTxPowerDbm));
    lteHelper->SetEnbAntennaModelType("ns3::ParabolicAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Beamwidth", DoubleValue(70));
    lteHelper->SetEnbAntennaModelAttribute("MaxAttenuation", DoubleValue(20.0));
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(macroEnbDlEarfcn));
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(macroEnbDlEarfcn + 18000));
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(macroEnbBandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(macroEnbBandwidth));
    NetDeviceContainer macroEnbDevs =
        lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice(macroEnbs);

    if (epc)
    {
        // this enables handover for macro eNBs                                                         EPC가 활성화된 경우, 매크로 eNB들을 위한 X2 인터페이스를 추가합니다.
        lteHelper->AddX2Interface(macroEnbs);
    }

    // HomeEnbs randomly indoor                                                                         홈 eNB들을 무작위로 설치하고 위치 및 장치를 설정합니다.

    Ptr<PositionAllocator> positionAlloc = CreateObject<RandomRoomPositionAllocator>();
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(homeEnbs);
    BuildingsHelper::Install(homeEnbs);
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(homeEnbTxPowerDbm));
    lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(homeEnbDlEarfcn));
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(homeEnbDlEarfcn + 18000));
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(homeEnbBandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(homeEnbBandwidth));
    lteHelper->SetEnbDeviceAttribute("CsgId", UintegerValue(1));
    lteHelper->SetEnbDeviceAttribute("CsgIndication", BooleanValue(true));
    NetDeviceContainer homeEnbDevs = lteHelper->InstallEnbDevice(homeEnbs);

    // home UEs located in the same apartment in which there are the Home eNBs                          홈 UE들을 같은 아파트 내에 무작위로 배치하고 위치 및 장치를 설정합니다.
    positionAlloc = CreateObject<SameRoomPositionAllocator>(homeEnbs);
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(homeUes);
    BuildingsHelper::Install(homeUes);
    // set the home UE as a CSG member of the home eNodeBs                                              홈 UE를 홈 eNB의 CSG 멤버로 설정합니다.
    lteHelper->SetUeDeviceAttribute("CsgId", UintegerValue(1));
    NetDeviceContainer homeUeDevs = lteHelper->InstallUeDevice(homeUes);

    // macro Ues                                                                                        매크로 사용
    NS_LOG_LOGIC("randomly allocating macro UEs in " << macroUeBox << " speedMin "                      // 랜덤으로 배치된 매크로 UEs   속도 최소
                                                     << outdoorUeMinSpeed << " speedMax "               // 속도 최대
                                                     << outdoorUeMaxSpeed);                             // 로 설정합니다.
    if (outdoorUeMaxSpeed != 0.0)
    {
        mobility.SetMobilityModel("ns3::SteadyStateRandomWaypointMobilityModel");

        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinX",
                           DoubleValue(macroUeBox.xMin));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinY",
                           DoubleValue(macroUeBox.yMin));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxX",
                           DoubleValue(macroUeBox.xMax));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxY",
                           DoubleValue(macroUeBox.yMax));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::Z", DoubleValue(ueZ));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MaxSpeed",
                           DoubleValue(outdoorUeMaxSpeed));
        Config::SetDefault("ns3::SteadyStateRandomWaypointMobilityModel::MinSpeed",
                           DoubleValue(outdoorUeMinSpeed));

        // this is not used since SteadyStateRandomWaypointMobilityModel                                이 부분은 사용되지 않습니다. SteadyStateRandomWaypointMObilityModel이
        // takes care of initializing the positions;  however we need to                                위치를 초기화하기 때문에 PositionAllocator를 재설정해야 합니다.
        // reset it since the previously used PositionAllocator                                         이전에 사용된 PositionAllocator (SameRoom)는 homeDeploymentRatio=0
        // (SameRoom) will cause an error when used with homeDeploymentRatio=0                          일 때 오류를 발생시킬 수 있습니다.
        positionAlloc = CreateObject<RandomBoxPositionAllocator>();
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(macroUes);

        // forcing initialization so we don't have to wait for Nodes to                                 노드가 시작되기 전에 위치가 할당되도록 초기화를 강제합니다.
        // start before positions are assigned (which is needed to
        // output node positions to file and to make AttachToClosestEnb work)
        for (auto it = macroUes.Begin(); it != macroUes.End(); ++it)
        {
            (*it)->Initialize();
        }
    }
    else
    {
        positionAlloc = CreateObject<RandomBoxPositionAllocator>();
        Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable>();
        xVal->SetAttribute("Min", DoubleValue(macroUeBox.xMin));
        xVal->SetAttribute("Max", DoubleValue(macroUeBox.xMax));
        positionAlloc->SetAttribute("X", PointerValue(xVal));
        Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable>();
        yVal->SetAttribute("Min", DoubleValue(macroUeBox.yMin));
        yVal->SetAttribute("Max", DoubleValue(macroUeBox.yMax));
        positionAlloc->SetAttribute("Y", PointerValue(yVal));
        Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable>();
        zVal->SetAttribute("Min", DoubleValue(macroUeBox.zMin));
        zVal->SetAttribute("Max", DoubleValue(macroUeBox.zMax));
        positionAlloc->SetAttribute("Z", PointerValue(zVal));
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(macroUes);
    }
    BuildingsHelper::Install(macroUes);

    NetDeviceContainer macroUeDevs = lteHelper->InstallUeDevice(macroUes);

    Ipv4Address remoteHostAddr;
    NodeContainer ues;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ipv4InterfaceContainer ueIpIfaces;
    Ptr<Node> remoteHost;
    NetDeviceContainer ueDevs;

    if (epc)
    {
        NS_LOG_LOGIC("setting up internet and remote host");                                            // 인터넷 및 원격 호스트 설정

        // Create a single RemoteHost                                                                   단일 원격 호스트 생성
        NodeContainer remoteHostContainer;
        remoteHostContainer.Create(1);
        remoteHost = remoteHostContainer.Get(0);
        InternetStackHelper internet;
        internet.Install(remoteHostContainer);

        // Create the Internet                                                                          인터넷 생성
        PointToPointHelper p2ph;
        p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
        p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
        p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
        Ptr<Node> pgw = epcHelper->GetPgwNode();
        NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
        Ipv4AddressHelper ipv4h;
        ipv4h.SetBase("1.0.0.0", "255.0.0.0");
        Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
        // in this container, interface 0 is the pgw, 1 is the remoteHost                               이 컨테이너에서 인터페이스 0은 P-G/W, 1은 원격 호스트입니다.
        remoteHostAddr = internetIpIfaces.GetAddress(1);

        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
        remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"),
                                                   Ipv4Mask("255.0.0.0"),
                                                   1);

        // for internetworking purposes, consider together home UEs and macro UEs                       인터네트워킹을 위해 home UEs와 매크로 UEs를 함께 고려합니다.
        ues.Add(homeUes);
        ues.Add(macroUes);
        ueDevs.Add(homeUeDevs);
        ueDevs.Add(macroUeDevs);

        // Install the IP stack on the UEs                                                              UEs에 IP 스택 설치
        internet.Install(ues);
        ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

        // attachment (needs to be done after IP stack configuration)                                   부착(IP 스택 구성 후에 수행해야 함)
        // using initial cell selection                                                                 초기 셀 선택 사용
        lteHelper->Attach(macroUeDevs);
        lteHelper->Attach(homeUeDevs);
    }
    else
    {
        // macro UEs attached to the closest macro eNB                                                  매크로 UEs를 가장 가까운 매크로 eNB에 연결합니다.
        lteHelper->AttachToClosestEnb(macroUeDevs, macroEnbDevs);

        // each home UE is attached explicitly to its home eNB                                          각 홈 UE는 해당 홈 eNB에 명시적으로 연결됩니다.
        NetDeviceContainer::Iterator ueDevIt;
        NetDeviceContainer::Iterator enbDevIt;
        for (ueDevIt = homeUeDevs.Begin(), enbDevIt = homeEnbDevs.Begin();
             ueDevIt != homeUeDevs.End();
             ++ueDevIt, ++enbDevIt)
        {
            // this because of the order in which SameRoomPositionAllocator                             SameRoomPositionAllocator가 UE를 배치한 순서 때문에
            // will place the UEs                                                                       이 작업이 필요합니다.
            if (enbDevIt == homeEnbDevs.End())
            {
                enbDevIt = homeEnbDevs.Begin();
            }
            lteHelper->Attach(*ueDevIt, *enbDevIt);
        }
    }

    if (epc)
    {
        NS_LOG_LOGIC("setting up applications");

        // Install and start applications on UEs and remote host                                        UEs와 원격 호스트에 응용 프로그램 설치 및 시작
        uint16_t dlPort = 10000;
        uint16_t ulPort = 20000;

        // randomize a bit start times to avoid simulation artifacts                                    시뮬레이션 아티팩트를 방지하기 위해 시작 시간을 약간 무작위화
        // (e.g., buffer overflows due to packet transmissions happening                                (예: 데이터 전송이 정확히 동시에 발생하여 버퍼 오버플로우 발생 방지)
        // exactly at the same time)
        Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
        if (useUdp)
        {
            startTimeSeconds->SetAttribute("Min", DoubleValue(0));
            startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));
        }
        else
        {
            // TCP needs to be started late enough so that all UEs are connected                        TCP는 모든 UEs가 연결된 후에만 시작해야 하므로 충분히 늦게 시작해야 합니다.
            // otherwise TCP SYN packets will get lost                                                  그렇지 않으면 TCP SYN 패킷이 손실될 수 있습니다.
            startTimeSeconds->SetAttribute("Min", DoubleValue(0.100));
            startTimeSeconds->SetAttribute("Max", DoubleValue(0.110));
        }

        for (uint32_t u = 0; u < ues.GetN(); ++u)
        {
            Ptr<Node> ue = ues.Get(u);
            // Set the default gateway for the UE                                                       UE의 기본 게이트웨이 설정
            Ptr<Ipv4StaticRouting> ueStaticRouting =
                ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

            for (uint32_t b = 0; b < numBearersPerUe; ++b)
            {
                ++dlPort;
                ++ulPort;

                ApplicationContainer clientApps;
                ApplicationContainer serverApps;

                if (useUdp)
                {
                    if (epcDl)
                    {
                        NS_LOG_LOGIC("installing UDP DL app for UE " << u);
                        UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
                        clientApps.Add(dlClientHelper.Install(remoteHost));
                        PacketSinkHelper dlPacketSinkHelper(
                            "ns3::UdpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                        serverApps.Add(dlPacketSinkHelper.Install(ue));
                    }
                    if (epcUl)
                    {
                        NS_LOG_LOGIC("installing UDP UL app for UE " << u);
                        UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
                        clientApps.Add(ulClientHelper.Install(ue));
                        PacketSinkHelper ulPacketSinkHelper(
                            "ns3::UdpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), ulPort));
                        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
                    }
                }
                else // use TCP                                                                         TCP 사용
                {
                    if (epcDl)
                    {
                        NS_LOG_LOGIC("installing TCP DL app for UE " << u);
                        BulkSendHelper dlClientHelper(
                            "ns3::TcpSocketFactory",
                            InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort));
                        dlClientHelper.SetAttribute("MaxBytes", UintegerValue(0));
                        clientApps.Add(dlClientHelper.Install(remoteHost));
                        PacketSinkHelper dlPacketSinkHelper(
                            "ns3::TcpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                        serverApps.Add(dlPacketSinkHelper.Install(ue));
                    }
                    if (epcUl)
                    {
                        NS_LOG_LOGIC("installing TCP UL app for UE " << u);
                        BulkSendHelper ulClientHelper("ns3::TcpSocketFactory",
                                                      InetSocketAddress(remoteHostAddr, ulPort));
                        ulClientHelper.SetAttribute("MaxBytes", UintegerValue(0));
                        clientApps.Add(ulClientHelper.Install(ue));
                        PacketSinkHelper ulPacketSinkHelper(
                            "ns3::TcpSocketFactory",
                            InetSocketAddress(Ipv4Address::GetAny(), ulPort));
                        serverApps.Add(ulPacketSinkHelper.Install(remoteHost));
                    }
                } // end if (useUdp)                                                                    

                Ptr<EpcTft> tft = Create<EpcTft>();
                if (epcDl)
                {
                    EpcTft::PacketFilter dlpf;
                    dlpf.localPortStart = dlPort;
                    dlpf.localPortEnd = dlPort;
                    tft->Add(dlpf);
                }
                if (epcUl)
                {
                    EpcTft::PacketFilter ulpf;
                    ulpf.remotePortStart = ulPort;
                    ulpf.remotePortEnd = ulPort;
                    tft->Add(ulpf);
                }

                if (epcDl || epcUl)
                {
                    EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
                    lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);
                }
                Time startTime = Seconds(startTimeSeconds->GetValue());
                serverApps.Start(startTime);
                clientApps.Start(startTime);

            } // end for b
        }
    }
    else // (epc == false)
    {
        // for radio bearer activation purposes, consider together home UEs and macro UEs               무선 베어러 활성화를 위해 홈 UEs와 매크로 UEs를 함께 고려합니다.
        NetDeviceContainer ueDevs;
        ueDevs.Add(homeUeDevs);
        ueDevs.Add(macroUeDevs);
        for (uint32_t u = 0; u < ueDevs.GetN(); ++u)
        {
            Ptr<NetDevice> ueDev = ueDevs.Get(u);
            for (uint32_t b = 0; b < numBearersPerUe; ++b)
            {
                EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
                EpsBearer bearer(q);
                lteHelper->ActivateDataRadioBearer(ueDev, bearer);
            }
        }
    }

    Ptr<RadioEnvironmentMapHelper> remHelper;
    if (generateRem)
    {
        PrintGnuplottableBuildingListToFile("buildings.txt");                                       // 건물, eNB, UE 목록을 파일로 출력
        PrintGnuplottableEnbListToFile("enbs.txt");
        PrintGnuplottableUeListToFile("ues.txt");

        remHelper = CreateObject<RadioEnvironmentMapHelper>();                                      // RadioEnvironmentMapHelper 객체 생성
        remHelper->SetAttribute("Channel", PointerValue(lteHelper->GetDownlinkSpectrumChannel()));  // 속성 설정
        remHelper->SetAttribute("OutputFile", StringValue("lena-dual-stripe.rem"));
        remHelper->SetAttribute("XMin", DoubleValue(macroUeBox.xMin));
        remHelper->SetAttribute("XMax", DoubleValue(macroUeBox.xMax));
        remHelper->SetAttribute("YMin", DoubleValue(macroUeBox.yMin));
        remHelper->SetAttribute("YMax", DoubleValue(macroUeBox.yMax));
        remHelper->SetAttribute("Z", DoubleValue(1.5));

        if (remRbId >= 0)                                                                           // REM에 데이터 채널 사용 여부 설정
        {
            remHelper->SetAttribute("UseDataChannel", BooleanValue(true));
            remHelper->SetAttribute("RbId", IntegerValue(remRbId));
        }

        remHelper->Install();                                                                       // REM 설치
        // simulation will stop right after the REM has been generated                              // REM 생성 후 시뮬레이션이 종료됩니다.
    }
    else
    {
        Simulator::Stop(Seconds(simTime));                                                          // REM 생성을 하지 않는 경우 지정된 시뮬레이션 시간 후 시뮬레이션 종료
    }

    lteHelper->EnableMacTraces();                                                                   // MAC 추적 활성화
    lteHelper->EnableRlcTraces();                                                                   // RLC 추적 활성화
    if (epc)
    {
        lteHelper->EnablePdcpTraces();
    }

    Simulator::Run();                                                                               // 시뮬레이션 실행

    // GtkConfigStore config;                                                                       GtkConfigStore 객체를 사용하여 추가적인 설정을 구성할 수 있음
    // config.ConfigureAttributes ();                                                               현재 코드에서는 사용되지 않음

    lteHelper = nullptr;                                                                            // LTE Helper 객체 및 시뮬레이션 객체 정리
    Simulator::Destroy();
    return 0;
}
