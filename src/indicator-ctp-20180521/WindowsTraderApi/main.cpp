#include "../ctp-trader/ctp-trader.h"
#include "../ctp-market/ctp-market.h"
#include "../ctp-market/ctp-int.h"

#include "../../include/thread/thread_event_impl.h"
#include "../../include/thread/ithread.h"

#include <conio.h>
#include <assert.h>
#include "../gtest/flags.h"

#include "../ctp-trader/ctp-trader-global.h"
#include "../ctp-log/ctp-log.h"

DEFINE_FLAG(charp, trade, "spread", "交易类型: spread/future ");

void clear_screen();

namespace ctp_spread{
    int main();
}
namespace ctp_market_app{
    int main();
}
namespace ctp_spread_test{
    int main();
}
namespace ctp_spread_print{
    int main();
}
namespace ctp_future{
    int main();
}
bool is_running();

int main(){
    if( is_running())//是否已经在运行
        return -1;

    dpu::Flags::Init();//解析命令行参数
    CTP_LOG_INIT("trader");

    if( stricmp(FLAG_trade, "market")==0)//市场行情: 接收行情
        return ctp_market_app::main();

    if( stricmp(FLAG_trade, "test")==0)//价差交易测试: 从文件读入价差数据,然后调用价差交易逻辑
        return ctp_spread_test::main();

    if( stricmp(FLAG_trade, "spread_print")==0)//价差: 打印价差
        return ctp_spread_print::main();

    if( stricmp(FLAG_trade, "future")==0)//期货交易: 单合约交易
        return ctp_future::main();

    if( stricmp(FLAG_trade, "spread")==0)//区间交易:价差
        return ctp_spread::main();

    return -1;
}
