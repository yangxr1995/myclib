
#include "args_temp.h"


ARG_DESC("mode", 1, 'm', "设置运行模式", char *, mode, NULL, "  [server|cmdline]")
ARG_DESC("front", 0, 'f', "设置前台执行", int, front, 0, NULL)
ARG_DESC("cmd", 1, 'c', "发送控制命令", char *, cmd, NULL, args_cmd_list())
ARG_DESC("data", 1, 'd', "控制命令的数据", char *, data, NULL, NULL)

#ifdef ARG_DESC
#undef ARG_DESC
#endif
