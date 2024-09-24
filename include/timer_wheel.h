#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "list_generic.h"
#include "event.h"

typedef struct timer_wheel_s timer_wheel_t;
typedef struct timer_solt_s timer_solt_t;
typedef struct timer_wheel_node_s timer_wheel_node_t;

typedef void (*timer_wheel_call_t)(void *);

struct timer_wheel_node_s {
    timer_wheel_call_t call;
    void *cb;
    bool is_repeat;
    _Atomic bool is_deleted;
    unsigned int cycle;
    long long tigger_ts;
    list_head_t list;
    timer_solt_t *solt;
};

struct timer_solt_s {
    pthread_spinlock_t lock;
    list_head_t nodes;
};

enum tick_mode {
    tick_mode_null,
    tick_mode_signal,
    tick_mode_timerfd,
};

struct timer_wheel_s {
    unsigned int solt_nb;
    unsigned int tick_interval;
    _Atomic(long long) cur_ts;
    enum tick_mode tick_mode;
    int timerfd;
    event_t ev;
    timer_solt_t *solts;
};

int timer_wheel_init(unsigned int solt_nb, unsigned int tick_interval);

timer_wheel_node_t *timer_wheel_add(unsigned int cycle, 
        bool is_repeat, timer_wheel_call_t call, void *cb);

int timer_wheel_del(timer_wheel_node_t *node);

int timer_wheel_start_by_sig();

void timer_wheel_tick();

int timer_wheel_start_by_timerfd();

void timer_wheel_destroy();

int timer_wheel_start_by_timerfd_ev(event_ctx_t *ev_ctx);

