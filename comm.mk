# 变量定义
CC = gcc
CFLAGS = -Iinclude -g -MMD -MP -O0 -DENABLE_DEBUG=1
LDFLAGS = -lpthread -rdynamic
SRC_DIR = src
OBJ_DIR = obj
BIN = a.out

# 查找所有的源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
# 将源文件转换为目标文件
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
# 将源文件转换为依赖文件
DEPS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.d,$(SRCS))

# 默认目标
all: $(BIN)

# 链接目标文件生成可执行文件
$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# 编译源文件为目标文件，并生成依赖文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)  # 确保目标目录存在
	$(CC) $(CFLAGS) -c $< -o $@

# 包含所有依赖文件
-include $(DEPS)

# 清理生成的文件
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(BIN)

.PHONY: all clean
