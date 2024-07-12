/*
 * Copyright (c) 2018 Fraunhofer ESK
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
 * Author: Vignesh Babu <ns3-dev@esk.fraunhofer.de>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LenaRadioLinkFailure");

// Global values to check the simulation                                                                시뮬레이션 도중 및 이후 시뮬레이션 동작을 확인하기 위한 전역 변수
// behavior during and after the simulation.
uint16_t counterN310FirsteNB = 0;        //!< Counter of N310 indications.                              N310 표시 횟수 카운터
Time t310StartTimeFirstEnb = Seconds(0); //!< Time of first N310 indication.                            첫 번째 N310 표시 시간
uint32_t ByteCounter = 0;                //!< Byte counter.                                             바이트 카운터
uint32_t oldByteCounter = 0;             //!< Old Byte counter,                                         이전 바이트 카운터

/**
 * Print the position of a UE with given IMSI.                                                          특정 IMSI를 가진 UE의 위치를 출력합니다.
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 */
void
PrintUePosition(uint64_t imsi)
{
    for (auto it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<LteUeNetDevice> uedev = node->GetDevice(j)->GetObject<LteUeNetDevice>();
            if (uedev)
            {
                if (imsi == uedev->GetImsi())
                {
                    Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                    std::cout << "IMSI : " << uedev->GetImsi() << " at " << pos.x << "," << pos.y
                              << std::endl;
                }
            }
        }
    }
}

/**
 * UE Notify connection established.                                                                    UE 연결 설정 알림 콜백 함수
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellid The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": connected to cell id " << cellid << " with RNTI " << rnti << std::endl;
}

/**
 * eNB Notify connection established.                                                                   eNB 연결 설정 알림 콜백 함수
 *
 * \param context The context.                                                                          컨텍스트
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB cell id " << cellId
              << ": successful connection of UE with IMSI " << imsi << " RNTI " << rnti
              << std::endl;
    // In this example, a UE should experience RLF at least one time in                                 이 예제에서는, 셀 1에 연결된 UE가 RLF를 경험할 때마다 N310 표시를 카운트
    // cell 1. For the case, when there is only one eNB with ideal RRC,                                 하여 처리합니다.
    // a UE might reconnects to the eNB multiple times due to more than
    // one RLF. To handle this, we reset the counter here so, even if the UE
    // connects multiple time to cell 1 we count N310
    // indication correctly, i.e., for each RLF UE RRC should receive
    // configured number of N310 indications.
    if (cellId == 1)
    {
        counterN310FirsteNB = 0;
    }
}

/// Map each of UE RRC states to its string representation.                                             UE RRC 상태를 문자열로 매핑
static const std::string g_ueRrcStateName[LteUeRrc::NUM_STATES] = {
    "IDLE_START",
    "IDLE_CELL_SEARCH",
    "IDLE_WAIT_MIB_SIB1",
    "IDLE_WAIT_MIB",
    "IDLE_WAIT_SIB1",
    "IDLE_CAMPED_NORMALLY",
    "IDLE_WAIT_SIB2",
    "IDLE_RANDOM_ACCESS",
    "IDLE_CONNECTING",
    "CONNECTED_NORMALLY",
    "CONNECTED_HANDOVER",
    "CONNECTED_PHY_PROBLEM",
    "CONNECTED_REESTABLISHING",
};

/**                                                                                                     UE RRC 상태를 문자열로 변환합니다.
 * \param s The UE RRC state.                                                                           s UE RRC 상태
 * \return The string representation of the given state.                                                주어진 상태의 문자열 표현
 */
static const std::string&
ToString(LteUeRrc::State s)
{
    return g_ueRrcStateName[s];
}

/**
 * UE state transition tracer.                                                                          UE 상태 전이 추적기
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 * \param oldState The old state.                                                                       이전 상태
 * \param newState The new state.                                                                       새로운 상태
 */
void
UeStateTransition(uint64_t imsi,
                  uint16_t cellId,
                  uint16_t rnti,
                  LteUeRrc::State oldState,
                  LteUeRrc::State newState)
{
    std::cout << Simulator::Now().As(Time::S) << " UE with IMSI " << imsi << " RNTI " << rnti
              << " connected to cell " << cellId << " transitions from " << ToString(oldState)
              << " to " << ToString(newState) << std::endl;
}

/**
 * eNB RRC timeout tracer.                                                                              eNB RRC 타임아웃 추적기
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param rnti The RNTI.                                                                                RNTI
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param cause The reason for timeout.                                                                 타임아웃 원인
 */
void
EnbRrcTimeout(uint64_t imsi, uint16_t rnti, uint16_t cellId, std::string cause)
{
    std::cout << Simulator::Now().As(Time::S) << " IMSI " << imsi << ", RNTI " << rnti
              << ", Cell id " << cellId << ", ENB RRC " << cause << std::endl;
}

/**
 * Notification of connection release at eNB.                                                           eNB에서 연결 해제 알림
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyConnectionReleaseAtEnodeB(uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << Simulator::Now() << " IMSI " << imsi << ", RNTI " << rnti << ", Cell id " << cellId
              << ", UE context destroyed at eNodeB" << std::endl;
}

/**
 * PHY sync detection tracer.                                                                           PHY 동기화 검출 추적기
 *
 * \param n310 310 data.                                                                                N310 데이터
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param rnti The RNTI.                                                                                RNTI
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param type The type.                                                                                유형
 * \param count The count.                                                                              카운트
 */
void
PhySyncDetection(uint16_t n310,
                 uint64_t imsi,
                 uint16_t rnti,
                 uint16_t cellId,
                 std::string type,
                 uint8_t count)
{
    std::cout << Simulator::Now().As(Time::S) << " IMSI " << imsi << ", RNTI " << rnti
              << ", Cell id " << cellId << ", " << type << ", no of sync indications: " << +count
              << std::endl;

    if (type == "Notify out of sync" && cellId == 1)                                                    // 셀 1에서 "out of sync" 유형의 PHY 문제가 발생하면 N310 표시 횟수를
    {                                                                                                   // 증가시킵니다.
        ++counterN310FirsteNB;
        if (counterN310FirsteNB == n310)
        {
            t310StartTimeFirstEnb = Simulator::Now();
        }
        NS_LOG_DEBUG("counterN310FirsteNB = " << counterN310FirsteNB);
    }
}

/**
 * Radio link failure tracer.                                                                           무선 링크 실패 추적기
 *
 * \param t310 310 data.                                                                                T310 데이터
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
RadioLinkFailure(Time t310, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << Simulator::Now() << " IMSI " << imsi << ", RNTI " << rnti << ", Cell id " << cellId
              << ", radio link failure detected" << std::endl
              << std::endl;

    PrintUePosition(imsi);                                                                              // RLF가 발생한 IMSI의 위치를 출력합니다.

    if (cellId == 1)                                                                                    // 셀 1에서 RLF가 발생했을 경우, T310 타이머가 올바른 시간에 만료되었는지
    {                                                                                                   // 확인합니다.
        NS_ABORT_MSG_IF((Simulator::Now() - t310StartTimeFirstEnb) != t310,
                        "T310 timer expired at wrong time");
    }
}

/**
 * UE Random access error notification.                                                                 UE 무작위 접근 오류 알림
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 */
void
NotifyRandomAccessErrorUe(uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " IMSI " << imsi << ", RNTI " << rnti
              << ", Cell id " << cellId << ", UE RRC Random access Failed" << std::endl;
}

/**
 * UE Connection timeout notification.                                                                  UE 연결 타임아웃 알림
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param cellId The Cell ID.                                                                           셀 ID
 * \param rnti The RNTI.                                                                                RNTI
 * \param connEstFailCount Connection failure count.                                                    연결 실패 횟수
 */
void
NotifyConnectionTimeoutUe(uint64_t imsi, uint16_t cellId, uint16_t rnti, uint8_t connEstFailCount)
{
    std::cout << Simulator::Now().As(Time::S) << " IMSI " << imsi << ", RNTI " << rnti
              << ", Cell id " << cellId << ", T300 expiration counter "
              << (uint16_t)connEstFailCount << ", UE RRC Connection timeout" << std::endl;
}

/**
 * UE RA response timeout notification.                                                                 UE RA 응답 타임아웃 알림
 *
 * \param imsi The IMSI.                                                                                IMSI 번호
 * \param contention Contention flag.                                                                   쟁탈 플래그
 * \param preambleTxCounter Preamble Tx counter.                                                        프리앰블 전송 카운터
 * \param maxPreambleTxLimit Max preamble Ts limit.                                                     최대 프리앰블 전송 제한
 */
void
NotifyRaResponseTimeoutUe(uint64_t imsi,
                          bool contention,
                          uint8_t preambleTxCounter,
                          uint8_t maxPreambleTxLimit)
{
    std::cout << Simulator::Now().As(Time::S) << " IMSI " << imsi << ", Contention flag "
              << contention << ", preamble Tx Counter " << (uint16_t)preambleTxCounter
              << ", Max Preamble Tx Limit " << (uint16_t)maxPreambleTxLimit
              << ", UE RA response timeout" << std::endl;
}

/**
 * Receive a packet.                                                                                    패킷 수신 핸들러
 *
 * \param packet The packet.                                                                            수신된 패킷
 */
void
ReceivePacket(Ptr<const Packet> packet, const Address&)
{
    ByteCounter += packet->GetSize();
}

/**
 * Write the throughput to file.                                                                        파일에 throughtput을 작성합니다.
 *
 * \param firstWrite True if first time writing.                                                        첫 번째 쓰기 여부
 * \param binSize Bin size.                                                                             빈 크기
 * \param fileName Output filename.                                                                     출력 파일 이름
 */
void
Throughput(bool firstWrite, Time binSize, std::string fileName)
{
    std::ofstream output;

    if (firstWrite)
    {
        output.open(fileName, std::ofstream::out);
        firstWrite = false;
    }
    else
    {
        output.open(fileName, std::ofstream::app);
    }

    // Instantaneous throughput every 200 ms                                                            200ms마다 순간적인 throughput을 계산하여 파일에 기록합니다.

    double throughput = (ByteCounter - oldByteCounter) * 8 / binSize.GetSeconds() / 1024 / 1024;
    output << Simulator::Now().As(Time::S) << " " << throughput << std::endl;
    oldByteCounter = ByteCounter;
    Simulator::Schedule(binSize, &Throughput, firstWrite, binSize, fileName);
}

/**
 * Sample simulation script for radio link failure.                                                     라디오 링크 실패를 검증하기 위한 LTE 네트워크 시뮬레이션 스크립트 예제입니다.
 * By default, only one eNodeB and one UE is considered for verifying                                   기본적으로 하나의 eNB와 하나의 UE만 고려되며, UE는 초기에 eNB의 커버리지에
 * radio link failure. The UE is initially in the coverage of                                           있으며 RRC 연결이 설정됩니다.
 * eNodeB and a RRC connection gets established.                                                        UE가 eNB에서 멀어짐에 따라 신호가 약해지며 동기화 비정상 표시가 발생합니다.
 * As the UE moves away from the eNodeB, the signal degrades                                            T310 타이머가 만료되면 라디오 링크가 실패로 간주되며 UE는 CONNECTED_NORMALLY
 * and out-of-sync indications are counted. When the T310 timer                                         상태를 떠나 다시 셀 선택을 수행합니다.
 * expires, radio link is considered to have failed and UE
 * leaves the CONNECTED_NORMALLY state and performs cell
 * selection again.
 *
 * The example can be run as follows:                                                                   다음과 같이 실행할 수 있습니다:
 *
 * ./ns3 run "lena-radio-link-failure --numberOfEnbs=1 --simTime=25"
 */
int
main(int argc, char* argv[])
{
    // Configurable parameters                                                                          // 설정 가능한 매개변수들
    Time simTime = Seconds(25);                                                                         // 시뮬레이션 총 시간 (초)
    uint16_t numberOfEnbs = 1;                                                                          // eNB 개수
    double interSiteDistance = 1200;                                                                    // 인접 사이트 간 거리 (미터)
    uint16_t n311 = 1;                                                                                  // 동기화 정상 표시 횟수
    uint16_t n310 = 1;                                                                                  // 동기화 비정상 표시 횟수
    Time t310 = Seconds(1);                                                                             // 라디오 링크 실패 탐지 타이머 (초)
    bool useIdealRrc = true;                                                                            // 이상적인 RRC 프로토콜 사용 여부
    bool enableCtrlErrorModel = true;                                                                   // 제어 오류 모델 활성화 여부
    bool enableDataErrorModel = true;                                                                   // 데이터 오류 모델 활성화 여부
    bool enableNsLogs = false;                                                                          // ns-3 로깅 활성화 여부

    CommandLine cmd(__FILE__);
    cmd.AddValue("simTime", "Total duration of the simulation (in seconds)", simTime);                  // 시뮬레이션 총 시간 (초 단위)
    cmd.AddValue("numberOfEnbs", "Number of eNBs", numberOfEnbs);                                       // eNB 개수
    cmd.AddValue("n311", "Number of in-synch indication", n311);                                        // 동기화 정상 표시 횟수
    cmd.AddValue("n310", "Number of out-of-synch indication", n310);                                    // 동기화 비정상 표시 횟수
    cmd.AddValue("t310", "Timer for detecting the Radio link failure (in seconds)", t310);              // 라디오 링크 실패 탐지 타이머 (초 단위)
    cmd.AddValue("interSiteDistance", "Inter-site distance in meter", interSiteDistance);               // 인접 사이트 간 거리 (미터)
    cmd.AddValue("useIdealRrc", "Use ideal RRC protocol", useIdealRrc);                                 // 이상적인 RRC 프로토콜 사용 여부
    cmd.AddValue("enableCtrlErrorModel", "Enable control error model", enableCtrlErrorModel);           // 제어 오류 모델 활성화 여부
    cmd.AddValue("enableDataErrorModel", "Enable data error model", enableDataErrorModel);              // 데이터 오류 모델 활성화 여부
    cmd.AddValue("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);                   // ns-3 로깅 활성화 여부 (디버그 빌드에서)
    cmd.Parse(argc, argv);

    if (enableNsLogs)
    {
        auto logLevel =
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
        LogComponentEnable("LteUeRrc", logLevel);
        LogComponentEnable("LteUeMac", logLevel);
        LogComponentEnable("LteUePhy", logLevel);

        LogComponentEnable("LteEnbRrc", logLevel);
        LogComponentEnable("LteEnbMac", logLevel);
        LogComponentEnable("LteEnbPhy", logLevel);

        LogComponentEnable("LenaRadioLinkFailure", logLevel);
    }

    uint16_t numberOfUes = 1;                                                                           // UE 개수
    uint16_t numBearersPerUe = 1;                                                                       // 각 UE 당 베어러 개수
    double eNodeB_txPower = 43;                                                                         // eNB 송신 전력 설정

    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(useIdealRrc));                       // LTE 헬퍼 및 EPC 헬퍼 생성
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                       BooleanValue(enableCtrlErrorModel));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                       BooleanValue(enableDataErrorModel));

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(60 * 1024));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    lteHelper->SetPathlossModelType(TypeId::LookupByName("ns3::LogDistancePropagationLossModel"));      // 패스로스 모델 설정
    lteHelper->SetPathlossModelAttribute("Exponent", DoubleValue(3.9));
    lteHelper->SetPathlossModelAttribute("ReferenceLoss",
                                         DoubleValue(38.57)); // ref. loss in dB at 1m for 2.025GHz
    lteHelper->SetPathlossModelAttribute("ReferenceDistance", DoubleValue(1));

    //----power related (equal for all base stations)----                                               // 전력 관련 설정
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(eNodeB_txPower));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(23));
    Config::SetDefault("ns3::LteUePhy::NoiseFigure", DoubleValue(7));
    Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(2));
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::ClosedLoop", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(true));

    //----frequency related----                                                                         // 주파수 관련 설정
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));   // 2120MHz
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100)); // 1930MHz
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(25)); // 5MHz
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(25)); // 5MHz

    //----others----                                                                                    // 스케줄러 설정
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
    Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.01));
    Config::SetDefault("ns3::PfFfMacScheduler::HarqEnabled", BooleanValue(true));

    Config::SetDefault("ns3::FfMacScheduler::UlCqiFilter", EnumValue(FfMacScheduler::SRS_UL_CQI));

    // Radio link failure detection parameters                                                          // RLF 감지 매개변수 설정
    Config::SetDefault("ns3::LteUeRrc::N310", UintegerValue(n310));
    Config::SetDefault("ns3::LteUeRrc::N311", UintegerValue(n311));
    Config::SetDefault("ns3::LteUeRrc::T310", TimeValue(t310));

    NS_LOG_INFO("Create the internet");
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    // Create a single RemoteHost0x18ab460                                                              // 인터넷 생성 및 설정
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NS_LOG_INFO("Create eNodeB and UE nodes");                                                          // eNB 및 UE 노드 생성
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(numberOfEnbs);
    ueNodes.Create(numberOfUes);

    NS_LOG_INFO("Assign mobility");                                                                     // 이동성 설정
    Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator>();

    for (uint16_t i = 0; i < numberOfEnbs; i++)
    {
        positionAllocEnb->Add(Vector(interSiteDistance * i, 0, 0));
    }
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAllocEnb);
    mobility.Install(enbNodes);

    Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator>();

    for (int i = 0; i < numberOfUes; i++)
    {
        positionAllocUe->Add(Vector(200, 0, 0));
    }

    mobility.SetPositionAllocator(positionAllocUe);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(ueNodes);

    for (int i = 0; i < numberOfUes; i++)
    {
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(30, 0.0, 0.0));
    }

    NS_LOG_INFO("Install LTE Devices in eNB and UEs and fix random number stream");                     // LTE 장치 설치 및 무작위 번호 스트림 고정
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;

    int64_t randomStream = 1;

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    randomStream += lteHelper->AssignStreams(enbDevs, randomStream);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);
    randomStream += lteHelper->AssignStreams(ueDevs, randomStream);

    NS_LOG_INFO("Install the IP stack on the UEs");
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    NS_LOG_INFO("Attach a UE to a eNB");                                                                // UE를 eNB에 연결
    lteHelper->Attach(ueDevs);

    NS_LOG_INFO("Install and start applications on UEs and remote host");                               // UE 및 원격 호스트에 응용 프로그램 설치 및 시작
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    DataRateValue dataRateValue = DataRate("18.6Mbps");

    uint64_t bitRate = dataRateValue.Get().GetBitRate();

    uint32_t packetSize = 1024; // bytes                                                                바이트

    NS_LOG_DEBUG("bit rate " << bitRate);

    double interPacketInterval = static_cast<double>(packetSize * 8) / bitRate;

    Time udpInterval = Seconds(interPacketInterval);

    NS_LOG_DEBUG("UDP will use application interval " << udpInterval.As(Time::S) << " sec");

    for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ApplicationContainer ulClientApps;
            ApplicationContainer ulServerApps;
            ApplicationContainer dlClientApps;
            ApplicationContainer dlServerApps;

            ++dlPort;
            ++ulPort;

            NS_LOG_LOGIC("installing UDP DL app for UE " << u + 1);                                     // 원격 호스트에 대한 UDP 애플리케이션 설치(DL)
            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
            dlClientHelper.SetAttribute("Interval", TimeValue(udpInterval));
            dlClientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            dlClientApps.Add(dlClientHelper.Install(remoteHost));

            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            dlServerApps.Add(dlPacketSinkHelper.Install(ue));

            NS_LOG_LOGIC("installing UDP UL app for UE " << u + 1);                                     // 원격 호스트에 대한 UDP 애플리케이션 설치(UL)
            UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
            ulClientHelper.SetAttribute("Interval", TimeValue(udpInterval));
            dlClientHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
            ulClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            ulClientApps.Add(ulClientHelper.Install(ue));

            PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            ulServerApps.Add(ulPacketSinkHelper.Install(remoteHost));

            Ptr<EpcTft> tft = Create<EpcTft>();                                                         // Dedicated EPS 베어러 활성화
            EpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            tft->Add(dlpf);
            EpcTft::PacketFilter ulpf;
            ulpf.remotePortStart = ulPort;
            ulpf.remotePortEnd = ulPort;
            tft->Add(ulpf);
            EpsBearer bearer(EpsBearer::NGBR_IMS);
            lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

            dlServerApps.Start(Seconds(0.27));                                                          // 애플리케이션 시작 시간 설정
            dlClientApps.Start(Seconds(0.27));
            ulServerApps.Start(Seconds(0.27));
            ulClientApps.Start(Seconds(0.27));
        } // end for b
    }
    NS_LOG_INFO("Enable Lte traces and connect custom trace sinks");                                    // LTE 추적 활성화 및 사용자 정의 추적 시간 스케줄링

    lteHelper->EnableTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));

    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",                         // 이벤트 핸들러 연결
                    MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteUeRrc/StateTransition",
                                  MakeCallback(&UeStateTransition));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteUeRrc/PhySyncDetection",
                                  MakeBoundCallback(&PhySyncDetection, n310));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteUeRrc/RadioLinkFailure",
                                  MakeBoundCallback(&RadioLinkFailure, t310));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteEnbRrc/NotifyConnectionRelease",
                                  MakeCallback(&NotifyConnectionReleaseAtEnodeB));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteEnbRrc/RrcTimeout",
                                  MakeCallback(&EnbRrcTimeout));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessError",
                                  MakeCallback(&NotifyRandomAccessErrorUe));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionTimeout",
                                  MakeCallback(&NotifyConnectionTimeoutUe));
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::LteUeNetDevice/"
                                  "ComponentCarrierMapUe/*/LteUeMac/RaResponseTimeout",
                                  MakeCallback(&NotifyRaResponseTimeoutUe));

    // Trace sink for the packet sink of UE                                                             UE 패킷 수신 추적
    std::ostringstream oss;
    oss << "/NodeList/" << ueNodes.Get(0)->GetId() << "/ApplicationList/0/$ns3::PacketSink/Rx";
    Config::ConnectWithoutContext(oss.str(), MakeCallback(&ReceivePacket));

    bool firstWrite = true;
    std::string rrcType = useIdealRrc ? "ideal_rrc" : "real_rrc";
    std::string fileName = "rlf_dl_thrput_" + std::to_string(enbNodes.GetN()) + "_eNB_" + rrcType;
    Time binSize = Seconds(0.2);
    Simulator::Schedule(Seconds(0.47), &Throughput, firstWrite, binSize, fileName);

    NS_LOG_INFO("Starting simulation...");                                                              // 시뮬레이션 시작

    Simulator::Stop(simTime);

    Simulator::Run();

    NS_ABORT_MSG_IF(counterN310FirsteNB != n310,                                                        // 테스트 결과 검증
                    "UE RRC should receive " << n310
                                             << " out-of-sync indications in Cell 1."                   // UE RRC는 셀 1에서 n개의 비동기 표시를 받아야 합니다. 현재 받은 횟수:
                                                " Total received = "
                                             << counterN310FirsteNB);

    Simulator::Destroy();

    return 0;
}
