/*
 *	The code is distributed under terms of the BSD license.
 *	Copyright (c) 2016 Alex Pankratov. All rights reserved.
 *
 *	http://swapped.cc/bsd-license
 */

#ifndef _DNS_H_
#define _DNS_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "arr.h"
#include "common.h"

/*
 *	Raw DNS packet header
 */
// 修改内存对齐方式为 1字节对齐
#pragma pack(push, 1)
// 下面对象定义都为1字节对齐

typedef struct dns_header
{
	uint16_t  id;     // 事务ID
	uint16_t  flags;  // FLAGS : QR(0请求 1响应) | Qpcode | AA | TC | | RD | RA | Z | rcode(0响应正确 1响应错误,服务器不能解析请求)
	uint16_t  qcount; // 问题计数
	uint16_t  acount; // 回答计数
	uint16_t  nscount;// 权威名称服务器计数
	uint16_t  arcount;// 附加信息计数

} dns_header;
// 查询问题区域
// 回答问题区域
// 权威名称服务器计数
// 附加信息计数

// 恢复以前的内存对齐方式
#pragma pack(pop)

// 0请求 1响应
#define DNS_GET_QR(hdr)     ( (htons((hdr)->flags) >> 15) & 0x01 )
// 0标准请求 1反向请求 2查询服务器状态
#define DNS_GET_OPCODE(hdr) ( (htons((hdr)->flags) >> 11) & 0x0F )
// 0响应正确 1响应异常
#define DNS_GET_RCODE(hdr)  ( htons((hdr)->flags) & 0x0F )

/*
 *	Parsed sections
 */
typedef struct dns_question
{
	char      name[256];// 查询的域名
	uint16_t  type;     // 类型，通常为A，表示标准服务器
	uint16_t  class_;   // 查询类，通常为0x1，表示互联网地址
} dns_question;


typedef struct dns_rr
{
	char      name[256]; // 回答的域名
	uint16_t  type;       
	uint16_t  class_;
	uint32_t  ttl;       // 生产时长
	uint16_t  len;       // 数据长度, ipv4则为4
	char    * data;      // 地址，比如ipv4
} dns_rr;

// hdr: 待解析的dns请求
// len: dns请求报文长度
// q_index: 一个请求可以带有多个问题，q_index为问题索引
// q: 返回一个问题
int dns_get_question(const dns_header * hdr, size_t len, size_t q_index, dns_question * q);

// hdr: 待解析的dns响应j
// len: dns响应报文长度
// a_index: 一个响应可以带有多个答案，a_index为响应索引
// a: 返回一个答案
int dns_get_answer(const dns_header * hdr, size_t len, size_t a_index, dns_rr * a);

void dump_dns_response(const dns_header * hdr, size_t len);
void dump_dns_response_safe(char *buf , int sz);

typedef struct {
    char *domain; // 只需要 Q1
    arr_t *ipv4;   // char *
} dns_resp_info_t;

inline static dns_resp_info_t *
dns_resp_info_new() 
{
    dns_resp_info_t *info;
    info = NEW(info);
    info->ipv4 = arr_new(3, sizeof(char *));
    return info;
}

inline static void 
dns_resp_info_free(dns_resp_info_t *info)
{
    arr_destroy(info->ipv4);
    free(info->domain);
    free(info);
}

dns_resp_info_t * dns_resp_parse(char *buf , int sz);

#pragma pack(push, 1)
typedef struct {
    struct ethhdr ethhdr;
    struct iphdr iphdr;
    struct udphdr udphdr;
    char payload[1024];
} udppkt;
#pragma pack(pop)



#endif

