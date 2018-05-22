#include "../ctp-trader/ctp-trader.h"
#include "../ctp-market/ctp-market.h"
#include "../ctp-market/ctp-market-mock.h"//从文件读取期货行情
#include "../ctp-market/ctp-int.h"
#include "../strategy/ctp-strategy-range2.h"

#include "../../include/thread/thread_event_impl.h"
#include "../../include/thread/ithread.h"
#include "../../include/string/utf8_string.h"

#include <conio.h>
#include <assert.h>
#include "../gtest/flags.h"

#include "../ctp-trader/ctp-trader-global.h"
#include "../ctp-log/ctp-log.h"

DECLARE_FLAG(charp, ctp_market_code);
DECLARE_FLAG(charp, ctp_trade_code);
DECLARE_FLAG(charp, range);

namespace ctp_spread_test{

//负责下单的类    
class my_ctp_trader{
public:

    std::vector<ctp_strategy_range2_ptr> strat_ranges;//区间交易策略


    //启动
    int start(){

        return 0;
    }

    //执行下单命令
    int order_spread2(ctp_order* o){

        if(! o->InstrumentID || strstr(o->InstrumentID,"&")==nullptr){
                assert(false);
                return -1;
        }
        CTP_PRINT("\n*****order: %s,  %s, %s, %d, %s\n",
            o->InstrumentID,
            o->buy?"buy ":"sell",
            o->open?"open ":"close",
            (int)o->price,
            o->time.c_str()
            );
        return 0;
    }
};

//负责提供期货行情
class my_ctp_market {
public:
    my_ctp_market():m_has_dot(false){}

    ctp_market_mock m; //从文件读取价差

    ctp_int m_last_spread1, m_last_spread2;//上次的价格
    ctp_int m_spread1, m_spread2;//本次价格

    bool m_has_dot;//打印 '.'

    std::function<void(const char* instrumentID, int spread1,int spread2,const char* s_time)> on_spread_changed;

    int start(){
        auto pthis =this;

        //设置接收期货行情的函数: 有新行情的时候, 这个函数会被调用
        m.set_market_callback([&,pthis](ctp_market_callback_data* d){

            //保存期货代码和买/卖价格
            m.set_price(d->InstrumentID, d->BuyPrice1, d->SellPrice1);

            double spread1 = -1000, spread2 = 1000;
            auto i = m.get_spread(spread1,spread2);//计算价差 = M1809 - M1901
            if( i != 0) {//计算出错

                //这里不直接退出函数,以便让后面打印代码继续执行

            }else{//价差计算成果

                //保存价差
                pthis->m_spread1.set((int)spread1);
                pthis->m_spread2.set((int)spread2);

                //发出价差改变通知
                pthis->on_spread_changed(d->InstrumentID,spread1,spread2,d->UpdateTime);
            }

            if( pthis->m_spread1.equal(pthis->m_last_spread1) &&
                pthis->m_spread2.equal(pthis->m_last_spread2) ){
                    //价差相等,和上次一样, 打印一个 '.'
                    pthis->m_has_dot =true;
                    printf(".");
                    return;
            }

            //更新价差
            pthis->m_last_spread1 = pthis->m_spread1;
            pthis->m_last_spread2 = pthis->m_spread2;

            //////////////////////////////////////////////////////////////////////////
            //打印价差信息
            char s_spread[128]={0};
            char msg[1024];
            sprintf(s_spread,"%-5d, %-5d", (int)spread1, (int)spread2);
            sprintf(msg,"%s%s,%16s,9003,%s.%03d, %8d, %-5d, %-5d, %s\n",
                "",//pthis->m_has_dot?"\n":"",
                "ctp",//pDepthMarketData->ExchangeID,
                d->InstrumentID,
                d->UpdateTime,
                d->UpdateMillisec,
                //pDepthMarketData->LastPrice,
                d->Volume,
                (int)d->BuyPrice1,//买
                (int)d->SellPrice1, //卖
                s_spread
                );
            CTP_PRINT(msg);

            pthis->m_has_dot=false;

        });
        return m.start();
    }
};


static int stategy_start( ctp_strategy_range2 &sr, my_ctp_trader& t)
{
    sr.set_market_codes(FLAG_ctp_market_code);
    sr.set_trade_code(FLAG_ctp_trade_code);

    //下单时调用的函数
    sr.set_on_order ( std::bind(&my_ctp_trader::order_spread2, &t, std::placeholders::_1 ));

    auto ret=sr.start();
    if(ret)return ret;

    return ret;
}

static std::vector<ctp_range> get_ranges(){
    std::vector<ctp_range>  ret;
    auto s_ranges = string_split(FLAG_range,";");
    assert(s_ranges.size()>0);
    if(s_ranges.size()==0) {
        goto err;
    }
    for(int i=0;i<s_ranges.size();i++){
        auto s = s_ranges[i];
        auto fields = string_split(s,",");
        if(fields.size()!=4){
            goto err;
        }
        ctp_range r;
        r.m_range_high.set(atoi(fields[0].c_str()));
        r.m_range_low .set(atoi(fields[1].c_str()));
        r.m_price_det .set(atoi(fields[2].c_str()));
        r.m_profit    .set(atoi(fields[3].c_str()));

        auto b = r.m_range_high.i > r.m_range_low.i &&
            r.m_price_det.i>0 &&
            r.m_profit.i >0;
        assert(b);
        if(!b)goto err;

        ret.push_back(r);
    }

    return ret;
err:
    ret.clear();
    CTP_LOG_ERROR("parse range error: "<< FLAG_range<< std::endl);
    exit(-1);
    return ret;
}

int main(void)
{
    //////////////////////////////////////////////////////////////////////////
    my_ctp_trader t;//交易账户

    //////////////////////////////////////////////////////////////////////////
    auto ranges = get_ranges();//从命令行取到区间定义
    int ret=0;
    for(int i=0;i<ranges.size();i++){//遍历区间定义

        ctp_strategy_range2_ptr sr(new ctp_strategy_range2());//新建区间交易策略

        //为区间设置 id
        char s_id[128]; sprintf(s_id,"rtest%d", i+1);
        sr->set_range ( ranges[i] );
        sr->set_id(s_id);

        //启动区间交易策略
        ret = stategy_start(*sr, t);
        if(ret)return ret;//失败

        t.strat_ranges.push_back(sr);//保存区间交易策略
    }


    //////////////////////////////////////////////////////////////////////////    
    ret = t.start();//启动交易账户
    if(ret)return ret;

    //////////////////////////////////////////////////////////////////////////
    my_ctp_market m;//行情价格

    //t.m_spread_sell = &m.m_spread1;
    //t.m_spread_buy = &m.m_spread2;
    //设置行情改变时的回调函数
    m.on_spread_changed =[&](const char* instrumentID,int spread_buy,int spread_sell,const char* spread_time){
        //行情改变了, 调用价差交易策略的处理函数,可能触发下单
        for(int i=0;i< t.strat_ranges.size();i++){
            //通知策略,价格改变了
            t.strat_ranges[i]->on_spread_change(instrumentID,spread_buy,spread_sell,spread_time);
        }
    };

    ret = m.start();//启动行情
    if(ret)return ret;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    printf("Press any key to quit.\n");
    getch(); exit(0);


    return 0;
}

}//namespace ctp_spread_test
