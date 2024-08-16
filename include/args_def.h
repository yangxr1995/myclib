
#include "args_temp.h"

ARG_DESC("once", 0, 'e', "设置程序单步执行", int, once, 0)
ARG_DESC("select", 1, 'q', "设置查询目标", char *, select_target, 0)

#ifdef ARG_DESC
#undef ARG_DESC
#endif
