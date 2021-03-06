// ctp-strategy.cpp : 定义 DLL 应用程序的导出函数。
//
#include <assert.h>
#include "ctp-strategy.h"

#include "../../include/string/utf8_string.h"
#include "../../include/config/bs_config.h"
#include <sstream>
#include <windows.h>
#include <algorithm>

#include "../ctp-log/ctp-log.h"
#include "../ctp-trader/ctp-trader-global.h"
#include "../gtest/flags.h"

DEFINE_FLAG( int, simnow, 1,"simnow模拟");

ctp_strategy::ctp_strategy()
{
}

ctp_strategy::~ctp_strategy()
{
}

void ctp_strategy::set_market_codes(const std::string& code)
{
    std::vector<std::string> dels;
    dels.push_back(";");
    dels.push_back(",");
    auto arr = string_split(code, dels);
    m_market_codes =arr;
}

int ctp_strategy::start()
{
    CTP_PRINT("*************** strategy [%s] 启动 ...\n", m_id.c_str());
    int ret = load_position();
    if(ret)return ret;

    return ret;
}

int ctp_strategy::on_spread_change( const char* instrumentID, int spread_buy, int spread_sell )
{
    return 0;
}

int ctp_strategy::on_notify(const ctp_trader_callback_data* d)
{
    if( d->event_id == event_OnRspUserLogin){
        m_trading_day = d->trading_day;
        m_session_id = d->session_id;
        assert(m_session_id.size()>0);
    }
    return 0;
}

int ctp_strategy::get_last_position( ctp_strategy_position* p, bool* has_postion )
{
    *has_postion = false;
    auto size = m_positions.m_positions.size();
    if(size == 0)
        return 0;//没有仓位

    for(int i=size-1;i>=0;i--){
        *p = m_positions.m_positions[i];
        if(! p->traded)// 挂单吗？
        {
            //判断交易日是否一致，过日的话，挂单全部被取消
            if(p->trading_day != m_trading_day)
                continue;
            else{//今天的挂单,等待成交
                *has_postion = true;
                return 0;
            }
        }else{
            //已经成交的单子
            if(p->open==false)//平仓单
            {
                *has_postion = false;//相当于没有单子
            }else
                *has_postion = true;//已开仓
            return 0;//有单子
        }
    }

    return 0;
}

std::string ctp_strategy::get_time()
{
    SYSTEMTIME st={0};
    GetLocalTime(&st);
    char s[256];
    sprintf(s, "%04d%02d%02d %02d:%02d:%02d.%03d", st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
    return s;
}

bool ctp_strategy::is_my_market_code(const std::string& c) const
{
    auto r = std::find(m_market_codes.begin(),m_market_codes.end(),c);
    return (r != m_market_codes.end());
}

std::string ctp_strategy::get_position_path()
{
    auto path = cosmos::bs_config::get_exe_folder();
    path += m_id + "-p.txt";
    return path;
}


int ctp_strategy::load_position()
{
    if(m_id.size()==0)return -1;
    auto path = get_position_path();
    if( ! cosmos::bs_config::file_exists(path))
        return 0;

    CTP_PRINT("load position from file : %s\n", path.c_str());

    std::vector<std::string> lines;

    auto ret = cosmos::bs_config::read_file_lines(path,lines);
    if(ret)return ret;

    ctp_strategy_positions positions;
    for(int i=0;i<lines.size();i++){
        auto p = ctp_strategy_position::parse(lines[i]);
        if(p)
            positions.m_positions.push_back(*p);
    }
    m_positions = positions;
    return 0;
}

int ctp_strategy::save_position()
{
    if(m_id.size()==0)return -1;

    std::ostringstream ss;
    for(int i=0;i<m_positions.m_positions.size();i++){
        auto line = m_positions.m_positions[i].to_string();
        ss<<line<<std::endl;
    }

    auto path = get_position_path();
    cosmos::bs_config::write_file(path, ss.str());
    return 0;
}

int ctp_strategy::do_order( const char* code,bool buy, bool open, int price ,const char*s_time)
{
	ctp_order p;
    p.InstrumentID = code;
    p.buy =buy;
    p.open = open;
    p.price = price;
    p.time = s_time;

    return do_order(&p);
}

int ctp_strategy::do_order( ctp_order* o)
{
    ctp_strategy_position p;
    p.buy = o->buy;
    p.open = o->open;
    p.price = o->price;
    p.trading_day = this->m_trading_day;
    p.session_id = this->m_session_id;

    std::string strTime(get_time());
    strTime += " ";
    strTime += o->time;
    p.time = strTime.c_str();//get_time();

    auto i = on_order(o);

    if(i==0)//成功
    {
        p.order_ref = o->order_ref;
        m_positions.m_positions.push_back(p);
        save_position();
    }
    return i;
}

std::shared_ptr<ctp_strategy_position> ctp_strategy_position::parse( const std::string& s )
{
    auto fields = string_split(s,",");
    if(fields.size() < 12 )
        return nullptr;
    for(int i=0;i<fields.size();i++)
        fields[i] = string_trim(fields[i]);

    std::shared_ptr<ctp_strategy_position> ret(new ctp_strategy_position());
    
    ret->buy = fields[0] == "buy";
    ret->open = fields[1]=="open";
    ret->price = atoi(fields[2].c_str());

    ret->changes = atoi(fields[3].c_str());
    ret->order_ref = fields[4].c_str();
    ret->status = atoi(fields[5].c_str());
    ret->order_sys_id = fields[6].c_str();

    ret->traded = atoi(fields[7].c_str());

    ret->trading_day = fields[8];
    ret->session_id = fields[9];

    ret->order_error_id = atoi(fields[10].c_str());
    
    ret->time = fields[11];
    return ret;
}

std::string ctp_strategy_position::to_string() const
{
    std::ostringstream ss;
    ss << (buy ? "buy ":"sell");
    ss<<", ";
    ss<< (open ? "open ":"close");
    ss<<", ";
    ss<<price;
    ss<<", ";
    ss<<changes;
    ss<<", ";
    ss<<order_ref;
    ss<<", ";
    ss<< (int)status;
    ss<<", ";
    ss<<order_sys_id;
    ss<<", ";
    ss<<traded;
    ss<<", ";

    ss<<trading_day;
    ss<<", ";

    ss<<session_id;
    ss<<", ";

    ss<<order_error_id;
    ss<<", ";

    ss<<time;
    ss<<", ";

    return ss.str();
}


bool ctp_strategy::is_trade_time( const char* s_time )
{
    if( FLAG_simnow )return true;//模拟交易

    std::string strTime(s_time);
    strTime += " ";

    //找到时间的分割符号 :
    auto p = strstr( strTime.c_str(),":");
    if(!p)return false;
    p-=2;
    //前两位应该是小时 数字
    char hour[3]={0};
    char minu[3]={0};
    char seco[3]={0};
    strncpy(hour,p,2);
    p+=3;
    strncpy(minu,p,2);
    p+=3;
    strncpy(seco,p,2);

    int i = atoi(hour);
    i = i*100 + atoi(minu);
    i = i*100+atoi(seco);

    if( i>= 90003 && i<=101457) return true;
    if( i>= 103003 && i<=112957) return true;
    if( i>= 133003 && i<=145957) return true;
    if( i>= 210003 && i<=232957) return true;
    return false;
}
