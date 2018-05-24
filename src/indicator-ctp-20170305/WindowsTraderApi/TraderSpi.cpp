#include <windows.h>
#include <iostream>
#include "TraderSpi.h"

#pragma warning(disable : 4996)
using namespace std;

extern CThostFtdcTraderApi* g_pTraderApi;
extern char* g_strTraderFront;
extern TThostFtdcBrokerIDType g_brokerId;
extern TThostFtdcInvestorIDType g_investorId;
extern TThostFtdcPasswordType g_passWord;
extern TThostFtdcInstrumentIDType g_instrumentId;
extern TThostFtdcPriceType g_limitPrice;
extern TThostFtdcDirectionType g_direction;
extern int iRequestID;

TThostFtdcFrontIDType g_frontId;
TThostFtdcSessionIDType g_sessionId;
TThostFtdcOrderRefType g_orderRef;

// 流控判断
bool IsFlowControl(int iResult)
{
    return ((iResult == -2) || (iResult == -3));
}

void CTraderSpi::OnFrontConnected()
{
    cerr << "--->>> " << "OnFrontConnected" << endl;
    ///用户登录请求
    ReqUserLogin();
}

void CTraderSpi::ReqUserLogin()
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, g_brokerId);
    strcpy(req.UserID, g_investorId);
    strcpy(req.Password, g_passWord);
    int iResult = g_pTraderApi->ReqUserLogin(&req, ++iRequestID);
    cerr << "--->>> 发送用户登录请求: " << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspUserLogin" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        // 保存会话参数
        g_frontId = pRspUserLogin->FrontID;
        g_sessionId = pRspUserLogin->SessionID;
        int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
        iNextOrderRef++;
        sprintf(g_orderRef, "%d", iNextOrderRef);
        ///获取当前交易日
        cerr << "--->>> 获取当前交易日 = " << g_pTraderApi->GetTradingDay() << endl;
        ///投资者结算结果确认
        ReqSettlementInfoConfirm();
    }
}

void CTraderSpi::ReqSettlementInfoConfirm()
{
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, g_brokerId);
    strcpy(req.InvestorID, g_investorId);
    int iResult = g_pTraderApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
    cerr << "--->>> 投资者结算结果确认: " << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspSettlementInfoConfirm" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询合约
        ReqQryInstrument();
    }
}

void CTraderSpi::ReqQryInstrument()
{
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.InstrumentID, g_instrumentId);
    while (true)
    {
        int iResult = g_pTraderApi->ReqQryInstrument(&req, ++iRequestID);
        if (!IsFlowControl(iResult))
        {
            cerr << "--->>> 请求查询合约: "  << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
            break;
        }
        else
        {
            cerr << "--->>> 请求查询合约: "  << iResult << ", 受到流控" << endl;
            Sleep(1000);
        }
    }
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryInstrument" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询合约
        ReqQryTradingAccount();
    }
}

void CTraderSpi::ReqQryTradingAccount()
{
    CThostFtdcQryTradingAccountField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, g_brokerId);
    strcpy(req.InvestorID, g_investorId);
    while (true)
    {
        int iResult = g_pTraderApi->ReqQryTradingAccount(&req, ++iRequestID);
        if (!IsFlowControl(iResult))
        {
            cerr << "--->>> 请求查询资金账户: "  << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
            break;
        }
        else
        {
            cerr << "--->>> 请求查询资金账户: "  << iResult << ", 受到流控" << endl;
            Sleep(1000);
        }
    } // while
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryTradingAccount" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///请求查询投资者持仓
        ReqQryInvestorPosition();
    }
}

void CTraderSpi::ReqQryInvestorPosition()
{
    CThostFtdcQryInvestorPositionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, g_brokerId);
    strcpy(req.InvestorID, g_investorId);
    strcpy(req.InstrumentID, g_instrumentId);
    while (true)
    {
        int iResult = g_pTraderApi->ReqQryInvestorPosition(&req, ++iRequestID);
        if (!IsFlowControl(iResult))
        {
            cerr << "--->>> 请求查询投资者持仓: "  << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
            break;
        }
        else
        {
            cerr << "--->>> 请求查询投资者持仓: "  << iResult << ", 受到流控" << endl;
            Sleep(1000);
        }
    } // while
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryInvestorPosition" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        ///报单录入请求
        ReqOrderInsert();
    }
}

void CTraderSpi::ReqOrderInsert()
{
    CThostFtdcInputOrderField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, g_brokerId);
    ///投资者代码
    strcpy(req.InvestorID, g_investorId);
    ///合约代码
    strcpy(req.InstrumentID, g_instrumentId);
    ///报单引用
    strcpy(req.OrderRef, g_orderRef);
    ///用户代码
    // TThostFtdcUserIDType UserID;
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向: 
    req.Direction = g_direction;
    ///组合开平标志: 开仓
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    req.LimitPrice = g_limitPrice;
    ///数量: 1
    req.VolumeTotalOriginal = 1;
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///GTD日期
    // TThostFtdcDateType GTDDate;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///止损价
    // TThostFtdcPriceType StopPrice;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///业务单元
    // TThostFtdcBusinessUnitType BusinessUnit;
    ///请求编号
    // TThostFtdcRequestIDType RequestID;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    int iResult = g_pTraderApi->ReqOrderInsert(&req, ++iRequestID);
    cerr << "--->>> 报单录入请求: " << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspOrderInsert" << endl;
    IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
    static bool ORDER_ACTION_SENT = false;  //是否发送了报单
    if (ORDER_ACTION_SENT)
        return;

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, pOrder->BrokerID);
    ///投资者代码
    strcpy(req.InvestorID, pOrder->InvestorID);
    ///报单操作引用
    // TThostFtdcOrderActionRefType OrderActionRef;
    ///报单引用
    strcpy(req.OrderRef, pOrder->OrderRef);
    ///请求编号
    // TThostFtdcRequestIDType RequestID;
    ///前置编号
    req.FrontID = g_frontId;
    ///会话编号
    req.SessionID = g_sessionId;
    ///交易所代码
    // TThostFtdcExchangeIDType ExchangeID;
    ///报单编号
    // TThostFtdcOrderSysIDType OrderSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
    // TThostFtdcPriceType LimitPrice;
    ///数量变化
    // TThostFtdcVolumeType VolumeChange;
    ///用户代码
    // TThostFtdcUserIDType UserID;
    ///合约代码
    strcpy(req.InstrumentID, pOrder->InstrumentID);

    int iResult = g_pTraderApi->ReqOrderAction(&req, ++iRequestID);
    cerr << "--->>> 报单操作请求: "  << iResult << ((iResult == 0) ? ", 成功" : ", 失败") << endl;
    ORDER_ACTION_SENT = true;
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspOrderAction" << endl;
    IsErrorRspInfo(pRspInfo);
}

///报单通知
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    cerr << "--->>> " << "OnRtnOrder"  << endl;
    if (IsMyOrder(pOrder))
    {
        if (IsTradingOrder(pOrder))
            ReqOrderAction(pOrder);
        else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
            cout << "--->>> 撤单成功" << endl;
    }
}

///成交通知
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    cerr << "--->>> " << "OnRtnTrade"  << endl;
}

void CTraderSpi:: OnFrontDisconnected(int nReason)
{
    cerr << "--->>> " << "OnFrontDisconnected" << endl;
    cerr << "--->>> Reason = " << nReason << endl;
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
    cerr << "--->>> " << "OnHeartBeatWarning" << endl;
    cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspError" << endl;
    IsErrorRspInfo(pRspInfo);
}

bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
    return bResult;
}

bool CTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->FrontID == g_frontId) &&
        (pOrder->SessionID == g_sessionId) &&
        (strcmp(pOrder->OrderRef, g_orderRef) == 0));
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
        (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}
