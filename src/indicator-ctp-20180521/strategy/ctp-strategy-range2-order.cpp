// ctp-strategy_range.cpp : 定义 DLL 应用程序的导出函数。
//

#include "ctp-strategy-range2-order.h"

#include "../../include/string/utf8_string.h"
#include "../../include/config/bs_config.h"
#include "../ctp-trader/ctp-trader-callback.h"

#include <assert.h>
#include "../gtest/flags.h"

int ctp_strategy_range2_order::start()
{
    return ctp_strategy_range::start();
}

int ctp_strategy_range2_order::on_spread_change( const char* instrumentID, int spread_buy, int spread_sell,const char* s_time )
{
    int ret = ctp_strategy_range::on_spread_change(instrumentID,spread_buy,spread_sell,s_time);
    if(ret != 0) return ret;

    return 0;
}


int ctp_strategy_range2_order::on_notify(const ctp_trader_callback_data* d)
{
    auto ret = ctp_strategy_range::on_notify(d);
    if(ret)return ret;

    //最后一次仓位
    ctp_strategy_position p;
    bool has_position=false;
    ret = get_last_position(&p,&has_position);
    if( ret)return ret;
    if(!has_position)//没有仓位
        return -1;

    if(auto o = d->to_OnRspOrderInsert()) //下单出错回报
    {
        if(p.session_id != m_session_id
            ||p.order_ref != o->OrderRef)//order ref是否一致
            return -1;
        p.changes++;
        p.order_error_id = o->ErrorID;

        m_positions.m_positions.push_back(p);
        save_position();
        return 0;
    }
    if(auto o = d->to_OnRtnOrder()) //交易所回报
    {
        if(p.order_sys_id.size() == 0)//没有 sys id
            if(p.session_id != m_session_id
                ||p.order_ref != o->OrderRef)//order ref是否一致
                return -1;

        //进一步检查 order sys id
        if(p.order_sys_id.size()>0)
            if(p.order_sys_id != o->OrderSysID)
                return -1;

        p.changes++;
        p.status = o->OrderStatus;
        p.order_sys_id = o->OrderSysID;

        m_positions.m_positions.push_back(p);
        save_position();
        return 0;
    }
    if(auto o = d->to_OnRtnTrade()) //交易所成交回报
    {
        if(p.order_sys_id != o->OrderSysID)//order sys id 是否一致
            return -1;
        p.changes++;
        p.traded = 1;

        m_positions.m_positions.push_back(p);
        save_position();
        return 0;
    }
    return ret;
}

int ctp_strategy_range2_order::do_buy_open()
{
    assert(m_range.m_range_low.valid);
    if(!m_range.m_range_low.valid)
        return -1;
    return do_order(get_trade_code().c_str(),true,true, m_range.m_range_low.i, m_time.c_str());
}

int ctp_strategy_range2_order::do_sell_open()
{
    assert(m_range.m_range_high.valid);
    if(!m_range.m_range_high.valid)
        return -1;
    return do_order(get_trade_code().c_str(),false,true, m_range.m_range_high.i, m_time.c_str());
}


int ctp_strategy_range2_order::do_close( const ctp_strategy_position& p )
{
    assert(p.open);
    if(!p.open) return -1;//上次仓位必须是开仓

    if(p.buy)//上次是买, 这次卖
    {
        return do_order(get_trade_code().c_str(),false,false,get_current_buy() - get_price_det(), m_time.c_str());
    }
    if(p.buy==false)//上次是卖, 这次买
    {
        return do_order(get_trade_code().c_str(),true,false, get_current_sell() + get_price_det(), m_time.c_str());
    }
    return -1;
}

int ctp_strategy_range2_order::do_close( const ctp_strategy_position& p,int profit )
{
    assert(p.open && profit>0);
    if(!p.open || profit<=0 ) return -1;//上次仓位必须是开仓, 必须有利润

    if(p.buy)//上次是买, 这次卖
    {
        return do_order(get_trade_code().c_str(),false,false, p.price + profit, m_time.c_str());
    }
    if(p.buy==false)//上次是卖, 这次买
    {
        return do_order(get_trade_code().c_str(),true,false, p.price - profit, m_time.c_str());
    }
    return -1;
}

int ctp_strategy_range2_order::on_spread_change()
{
    int ret = ctp_strategy_range::on_spread_change();
    if(ret != 0) return ret;

    //////////////////////////////////////////////////////////////////////////
    //逻辑：
    
    //////////////////////////////////////////////////////////////////////////
    //是否已经有仓位?
    ctp_strategy_position p;
    bool has_position = false;
    ret = get_last_position(&p, &has_position);
    if(ret) return ret;


    if( ! has_position ){//没有仓位
        //1.没有仓位就挂单
        if(m_is_buy)//只买
        {
            //挂买单
            ret = do_buy_open();
        }else{//只卖
            //挂买单
            ret = do_sell_open();
        }
    }else{//有仓位
        //2.有单就等待成交
        if( p.traded == false)//今天的挂单，还没成交，继续等待
        {
        }else{//已经成交
            //必须是开仓的单子
            assert(p.open);
            if(!p.open)return -1;

            //5个点就平仓
            //计算利润
            //auto profit = calc_profit(p);
            //if(profit >= m_range.m_profit.i){
            //    ret = do_close(p);
            //    return ret;
            //}

            //直接在利润上挂断平仓
            assert(m_range.m_profit.valid && m_range.m_profit.i>0);
            if( ! m_range.m_profit.valid || m_range.m_profit.i<=0)
                return -1;
            do_close(p, m_range.m_profit.i);
        }
    }

    //3.开仓成交后挂平仓单
    //4.goto 1

    return ret;
}
