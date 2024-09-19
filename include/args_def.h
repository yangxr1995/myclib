
#include "args_temp.h"

ARG_DESC("reader", 1, 'r', "设置读者数量", int, nb_reader, 1)
ARG_DESC("writer", 1, 'w', "设置写着数量", int, nb_writer, 1)
ARG_DESC("deleter", 1, 'd', "设置删除者数量", int, nb_deleter, 1)
ARG_DESC("mode", 1, 'm', "锁方法", char *, mode, "spinlock")
ARG_DESC("test", 1, 't', "功能测试", char *, test, NULL)

#ifdef ARG_DESC
#undef ARG_DESC
#endif
