
#include "args_temp.h"

ARG_DESC("mode", 1, 'm', "设置运行模式", char *, mode, "null", "  [server|cmdline]")
ARG_DESC("front", 0, 'f', "设置前台执行", int, front, 0, NULL)
ARG_DESC("cmd", 1, 'c', "发送控制命令", char *, cmd, 0, args_cmd_list())
ARG_DESC("name", 1, 'n', "控制指令的参数1", char *, name, 0, "null")
ARG_DESC("age", 1, 'a', "控制指令的参数2", char *, age, 0, "null")

#ifdef ARG_DESC
#undef ARG_DESC
#endif
