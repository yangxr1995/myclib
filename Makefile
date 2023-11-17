
all:test
#test:src/memchk.c src/main.c src/assert.c src/mm_pool.c src/fmt.c
#	mips-linux-gcc -I./include $^ -o $@ -rdynamic -funwind-tables

# 使用-Wl,-Map=./map.txt , 方便addr2line将地址信息转换为文件行信息
# 如 func1,
# grep func1 ./map.txt 得到func1的入口地址如 0x000001
# 运行程序，打印栈信息，得到func1+0x2
# 目标地址为 0x3
# addr2line -e ./test 0x3 -Cfsi
test:src/memchk.o src/main.o src/assert.o src/mm_pool.o src/fmt.o src/debug.o src/logger.o ./src/timer_list.o src/event.o
	$(CC) $^ -o $@ -rdynamic -Wl,-Map=./map.txt -finstrument-functions -no-pie -lpthread

# 使用 -g后，获得的栈信息的地址可以直接给addr2line转换
%.o:%.c
	$(CC) -O0 -I./include -c $^ -o $@ -funwind-tables -g3 -finstrument-functions


clean:
	rm -f test src/*.o
