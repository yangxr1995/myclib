#include <stdatomic.h>
#include <stddef.h>

#include "mcslock.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)

enum { MCS_PROCEED = 0, MCS_WAIT = 1 };

#if defined(__i386__) || defined(__x86_64__)
#define spin_wait() __builtin_ia32_pause()
#elif defined(__aarch64__)
#define spin_wait() __asm__ __volatile__("isb\n")
#else
#define spin_wait() ((void) 0)
#endif

static inline void wait_until_equal_u8(_Atomic uint8_t *loc,
                                       uint8_t val,
                                       memory_order mm)
{
    while (atomic_load_explicit(loc, mm) != val)
        spin_wait();
}

void mcslock_init(mcslock_t *lock)
{
    atomic_init(lock, NULL);
}

void mcslock_lock(mcslock_t *lock, mcsnode_t *node)
{
    atomic_init(&node->next, NULL);

    /*用node做尾节点，并返回上轮的尾节点到prev*/
    mcsnode_t *prev =
        atomic_exchange_explicit(lock, node, memory_order_acq_rel);

    /*上轮没有尾节点，说明没有其他线程在竞争锁,直接获得锁*/
    if (LIKELY(!prev))
        return;

    /*有其他更早的线程在竞争,排队在他们后面*/

    /*将自己置为自旋状态*/
    atomic_store_explicit(&node->wait, MCS_WAIT, memory_order_release);
    /*排队在其他线程后面*/
    atomic_store_explicit(&prev->next, node, memory_order_release);

    /*自旋等待直到其他线程让自己退出自旋*/
    wait_until_equal_u8(&node->wait, MCS_PROCEED, memory_order_acquire);
}

void mcslock_unlock(mcslock_t *lock, mcsnode_t *node)
{
    mcsnode_t *next;

    /*node为队列的首节点,且只有一个线程在调用unlock*/

    /*检查本节点，判断是否有其他线程在等待锁*/
    if ((next = atomic_load_explicit(&node->next, memory_order_acquire)) ==
        NULL) {

        /*没有其他线程在等待，尝试释放锁，即将 lock置为NULL*/

        /*在lock时，先修改*lock变量所以必须先检查*lock*/

        /*使用临时变量防止node被修改*/
        mcsnode_t *tmp = node;

        if (atomic_compare_exchange_strong_explicit(
                lock, &tmp, NULL, memory_order_release, memory_order_relaxed)) {
            /*没有其他线程在等待，并成功释放锁*/
            return;
        }

        /*等待追加的节点完成链接*/
        while ((next = atomic_load_explicit(&node->next,
                                            memory_order_acquire)) == NULL)
            spin_wait();
    }

    /*有其他线程在等待,让其退出自旋,以获得锁*/
    atomic_store_explicit(&next->wait, MCS_PROCEED, memory_order_release);
}
