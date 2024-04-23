
#CC:=/opt/toolchains/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/bin/arm-linux-gcc
#CC:=arm-openwrt-linux-gcc
CC:=gcc

all:test hook.so
#test:src/memchk.c src/main.c src/assert.c src/mm_pool.c src/fmt.c
#	mips-linux-gcc -I./include $^ -o $@ -rdynamic -funwind-tables

# 要使用栈打印需要加 -funwind-tables
# 要使用 finstrument-functions 需要添加
# CFLAGS += -finstrument-functions -funwind-tables
# LDFLAGS += -rdynamic -no-pie
# 注意 addr2line 的目标elf文件可以没有debug info，不能进行 strip
# 否则连接属性为LOCAL的符号会找不到
# 使用 readelf -s bin | grep LOCAL 
# 可以可以检查 LOCAL 符号是否被删除, 需要确保其存在

# 使用-Wl,-Map=./map.txt , 方便addr2line将地址信息转换为文件行信息
# 如 func1,
# grep func1 ./map.txt 得到func1的入口地址如 0x000001
# 运行程序，打印栈信息，得到func1+0x2
# 目标地址为 0x3
# addr2line -e ./test 0x3 -Cfsi
test:src/memchk.o src/main.o src/assert.o src/mm_pool.o src/fmt.o src/debug.o src/logger.o ./src/timer_list.o src/event.o src/thread_pool.o src/arr.o ./src/memchk.o src/task.o src/crypto.o
	$(CC) $^ -o $@ -rdynamic -Wl,-Map=./map.txt -no-pie -lpthread -lcrypto -lssl

# 使用 -g后，获得的栈信息的地址可以直接给addr2line转换
%.o:%.c
	$(CC) -O0 -I./include -c $^ -o $@ -funwind-tables -g3 -D_FORTIFY_SOURCE=2  -fstack-protector

hook.so: ./src/hook.c ./src/debug.c
	${CC} -D__HOOK_LIB -fPIC -shared -I./include -o hook.so $^ -ldl -g -O0

clean:
	rm -f test src/*.o hook.so
