#include <linux/if_ether.h>
#include <linux/ip.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/icmp.h>
#include <unistd.h>
// #include <net/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "dumphex.h"
 
int tun_alloc(char *dev, int flags) {
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";
 
    // 打开字符设备
    if( (fd = open(clonedev, O_RDWR)) < 0 ) {
        log_err("open %s", clonedev);
        return fd;
    }
    
    // IFF_TUN : 创建tun设备
    // IFF_TAP : 创建tap设备
    //  IFF_NO_PI : 只需要纯IP数据，否则开头会添加4个字节的额外数据（2个标志字节和2个协议字节）
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    
    // 如果没有指定设备名，内核会自动生成
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    
    // 创建设备
    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
        close(fd);
        return err;
    }
    
    // 返回设备名
    strcpy(dev, ifr.ifr_name);
    
    // 返回打开的新设备的fd
    return fd;
}
 
int test_main(){
    int tun_fd,nread;
    unsigned char buffer[2000];
    char tun_name[IFNAMSIZ];
    
    /* Connect to the device */
    strcpy(tun_name, "tun77");
    printf("%s %d\n", __func__, __LINE__);
    tun_fd = tun_alloc(tun_name, IFF_TAP);  /* tun interface */
    if(tun_fd < 0){
        perror("Allocating interface");
        exit(1);
    }
    printf("%s %d\n", __func__, __LINE__);
    system("ip addr add 172.11.22.33/24 dev tun77");
    system("ip link set tun77 up");
    printf("%s %d\n", __func__, __LINE__);

    struct ethhdr *eth;
 
    /* Now read data coming from the kernel */
    while(1) {
        /* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
        nread = read(tun_fd,buffer,sizeof(buffer));
        if(nread < 0) {
            perror("Reading from interface");
            close(tun_fd);
            exit(1);
        }

        /* Do whatever with the data */
        printf("Read %d bytes from device %s\n", nread, tun_name);
        dumphex(stdout, NULL, buffer, nread);

    }
}



