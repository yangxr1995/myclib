#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>






#include "event.h"
#include "str.h"
#include "assert.h"
#include "mm_pool.h"
#include "fmt.h"
#include "debug.h"
#include "logger.h"
#include "timer_list.h"
#include "thread_pool.h"
#include "task.h"
#include "crypto.h"
#include "com_msg.h"

int test_aw(int, char **);

int main(int argc, char** argv)
{
    test_aw(argc, argv);
    while (1) {
        sleep(1);
    }
    return 0;
}
