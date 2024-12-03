#pragma once

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <linux/if.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/if_ether.h>

#define NEW(n)  ({ \
    void *___ptr = malloc(sizeof(*n)); \
    memset(___ptr, 0, sizeof(*n)); \
    ___ptr; \
})

#define FREE(p)   \
do { \
    if (p) { \
        free(p); \
        p = NULL; \
    } \
} while (0)

#define ARR_NB(a) \
    sizeof(a) / sizeof(a[0])

inline static int
ethhdr_to_str(struct ethhdr *eth, char *info)
{
    return sprintf(info,
            "------ETH------\n"
            "目的MAC地址:%02x:%02x:%02x:%02x:%02x:%02x\n"
            "源MAC地址  :%02x:%02x:%02x:%02x:%02x:%02x\n"
            "协议       :0x%04X\n",
            eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5],
            eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
            ntohs(eth->h_proto));
}

inline static int 
udphdr_to_str(struct udphdr *udph, char *info)
{
    return sprintf(info, 
            "-------UDP-------\n"
            "源端口(2字节):%u\n"
            "目的端口(2字节):%u\n"
            "总长度(2字节):%u\n"
            "校验和(2字节):0x%04x\n",
            ntohs(udph->source),
            ntohs(udph->dest),
            ntohs(udph->len),
            udph->check);
}

inline static int 
iphdr_to_str(struct iphdr *iph, char *info)
{
    char src_ip[16] = {0}; char dst_ip[16] = {0};
    inet_ntop(AF_INET, &iph->saddr, src_ip, 16);
    inet_ntop(AF_INET, &iph->daddr, dst_ip, 16);
    return sprintf(info,
            "-----IP-----\n"
            "版本号(4位):%u\n"
            "头部长度(4位):%u\n"
            "服务类型(8位):%u\n"
            "总长度(16位):%u\n"
            "标识(16位):%u\n"
            "3位标志 + 13为片偏移(16位):0x%04x\n"
            "生存时间(8位):%u\n"
            "协议号(8位):%u\n"
            "头部校验和(16位):0x%x\n"
            "源IP地址:%s\n"
            "目的IP地址:%s\n",
            iph->version, iph->ihl, iph->tos, ntohs(iph->tot_len),
            ntohs(iph->id), ntohs(iph->frag_off),
            iph->ttl, iph->protocol, iph->check,
            src_ip,
            dst_ip);
}

inline static int do_system(char *fmt, ...)
{
    char cmd[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);
    log_debug("%s", cmd);
    return system(cmd);
}


