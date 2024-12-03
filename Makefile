# CROSS_COMPILE := /opt/toolchains/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/bin/arm-linux-
CC = $(CROSS_COMPILE)gcc

CFLAGS = -Iinclude -MMD -MP -O2
CFLAGS += -funwind-tables

# CFLAGS += -fsanitize=address
# LDFLAGS +=  -lasan -static-libasan
#
LDFLAGS += -lpthread -rdynamic -no-pie
# LDFLAGS += -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=strdup
SRC_DIR = src
OBJ_DIR = obj
BIN = sercmd

SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS = src/args.c  src/assert.c   src/com_msg.c  src/logger.c  src/sercmd.c  src/timer_wheel.c \
src/arr.c   src/cmdline.c  src/event.c    src/main.c    src/task.c    src/trie.c
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.d,$(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(BIN)

.PHONY: all clean
