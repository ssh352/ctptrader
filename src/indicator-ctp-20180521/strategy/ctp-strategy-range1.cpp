// ctp-strategy_range.cpp : 定义 DLL 应用程序的导出函数。
//

#include "ctp-strategy-range1.h"

#include "../../include/string/utf8_string.h"
#include "../../include/config/bs_config.h"

#include <assert.h>
#include "../gtest/flags.h"

int ctp_strategy_range1::start()
{
    return ctp_strategy_range::start();
}

int ctp_strategy_range1::on_spread_change( const char* instrumentID, int spread_buy, int spread_sell,const char* s_time )
{
    int ret = ctp_strategy_range::on_spread_change(instrumentID,spread_buy,spread_sell,s_time);
    if(ret != 0) return ret;

    return 0;
}


int ctp_strategy_range1::do_buy_open()
{
    return do_order(get_trade_code().c_str(),true,true, get_current_sell() + get_price_det(), get_current_sell(), m_time.c_str());
}

int ctp_strategy_range1::do_sell_open()
{
    return do_order(get_trade_code().c_str(),false,true, get_current_buy() - get_price_det(), get_current_buy(), m_time.c_str());
}


int ctp_strategy_range1::do_close( const ctp_strategy_position& p )
{
    assert(p.open);
    if(!p.open) return -1;//上次仓位必须是开仓

    if(p.buy)//上次是买, 这次卖
    {
        return do_order(get_trade_code().c_str(),false,false,get_current_buy() - get_price_det(),get_current_buy(), m_time.c_str());
    }
    if(p.buy==false)//上次是卖, 这次买
    {
        return do_order(get_trade_code().c_str(),true,false, get_current_sell() + get_price_det(), get_current_sell(),  m_time.c_str());
    }
    return -1;
}

int ctp_strategy_range1::on_spread_change()
{
    int ret = ctp_strategy_range::on_spread_change();
    if(ret != 0) return ret;

    //////////////////////////////////////////////////////////////////////////
    //是否已经有仓位?
    ctp_strategy_position p;
    bool has_position = false;
    ret = get_position(&p, &has_position);
    if(ret) return ret;

    if( ! has_position ){//没有仓位,等待开仓
        //价格是否在价格范围内面?

        if( price_gt_high())//价格超过上界, 逆势开空
        {
            ret = do_sell_open();
            return ret;
        }
        if( price_lt_low())//价格低于下界, 逆势开多
        {
            ret = do_buy_open();
            return ret;
        }
    }

    if(has_position){//有仓位的话,等待平仓
        //5个点就平仓
        //计算利润
        auto profit = calc_profit(p);
        if(profit >= m_range.m_profit.i){
            ret = do_close(p);
            return ret;
        }
    }
    return 0;
}
