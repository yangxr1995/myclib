ifeq ($(APP_NAME),)

PROJECT := $(shell ls ./app/)
TOP_DIR := $(shell pwd)
OBJ_DIR := $(TOP_DIR)/obj
LIB_DIR := $(TOP_DIR)/lib

# export CFLAGS += -DDEBUG_WRAP_MALLOC -g -O0
export CFLAGS += -g -O0
# export LDFLAGS += -Wl,--wrap=calloc -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=realloc -rdynamic -no-pie
export LDFLAGS += -rdynamic -no-pie

export TOP_DIR OBJ_DIR LIB_DIR APP_SRCS APP_DIR APP_NAME APP_INCLUDE_DIR

.PHONY: all clean $(PROJECT)

all:$(PROJECT)

$(PROJECT):
	$(MAKE) -C ./app/$@

clean:
	rm -rf $(OBJ_DIR)

hook-malloc: hook/hook_malloc.c
	${CC} -fPIC -shared -I./include -o libhook_malloc.so $^ -ldl -g -O0 -std=gnu11 -lpthread -Wl,--version-script=version.script

else

# LIB_SRCS = args.c  assert.c   com_msg.c  logger.c  sercmd.c  timer_wheel.c arr.c  cmdline.c  event.c  task.c  trie.c
# LIB_SRCS = $(patsubst %.c,$(LIB_DIR)/%.o,$(LIB_SRCS))
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)

OBJS = $(patsubst $(LIB_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
OBJS += $(patsubst $(APP_DIR)/%.c,$(OBJ_DIR)/%.o,$(APP_SRCS))

DEPS = $(patsubst $(LIB_DIR)/%.c,$(OBJ_DIR)/%.d,$(LIB_SRCS))
DEPS += $(patsubst $(APP_DIR)/%.c,$(OBJ_DIR)/%.d,$(APP_SRCS))

CFLAGS += -I$(TOP_DIR)/include
ifneq ($(APP_INCLUDE_DIR),)
CFLAGS += -I$(APP_INCLUDE_DIR)
endif

LDFLAGS += -lpthread -lrt -ldl

# CFLAGS += -fsanitize=address
# LDFLAGS += -lasan -static-libasan


# CFLAGS += -finstrument-functions -g -O0 -funwind-tables
# LDFLAGS += -rdynamic -funwind-tables

# CFLAGS += -fsanitize=address
# LDFLAGS += -lasan -static-libasan

all:$(APP_NAME)

CC := gcc
# CC := /opt/toolchains/crosstools-arm-gcc-9.2-linux-4.19-glibc-2.30-binutils-2.32/bin/arm-linux-gcc

$(APP_NAME):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(APP_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@


-include $(DEPS)

endif

