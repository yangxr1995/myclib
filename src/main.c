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


int test_main();


int raw_init() {
    int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        printf("raw sock create error");
        return -1;
    }

    return sockfd;
}

int test_cmsg();

int main(int argc, char** argv)
{
    int fd;


    test_cmsg();

    enable_console_log();

    fd = raw_init();


    char buf[2000];
    int n;

    while (1) {

        n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n < 0) {
            log_err("recvfrom");
            exit (1);
        }
        log_info("recv : %d", n);
    }

    return 0;
}
