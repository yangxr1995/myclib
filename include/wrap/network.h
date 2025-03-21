#ifdef __cplusplus
extern "C" {
#endif

#ifdef WRAP_DEFINE

#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <arpa/inet.h>
#include <sys/socket.h>
#include "wrap/utils.h"

wrap_define(int, socket, int domain, int type, int protocol)
{
    int ret = __real_socket(domain, type, protocol);
    log_wrap_lib_info("%d = socket()", ret);
    return ret;
}

wrap_define(int, bind, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = __real_bind(sockfd, addr, addrlen);
    log_wrap_lib_info("%d = bind(fd[%d])", ret, sockfd);
    return ret;
}

wrap_define(int, listen, int sockfd, int backlog)
{
    log_wrap_lib_info("listen(fd[%d], backlog[%d])", sockfd, backlog);
    return __real_listen(sockfd, backlog);
}

wrap_define(int, connect, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret = __real_connect(sockfd, addr, addrlen);
    log_wrap_lib_info("%d = connect(fd[%d])", ret, sockfd);
    return ret;
}

wrap_define(ssize_t, send, int sockfd, const void *buffer, size_t length, int flags)
{
    ssize_t ret = __real_send(sockfd, buffer, length, flags);
    log_wrap_lib_info("%d = send(sockfd[%d], buffer[%p], length[%ld], flags[%d])", 
            (int)ret, sockfd, buffer, length, flags);
    return ret;
}

wrap_define(ssize_t,  recv, int sockfd, void *buf, size_t len, int flags)
{
    ssize_t ret = __real_recv(sockfd, buf, len, flags);
    log_wrap_lib_info("%d = recv(sockfd[%d])", (int)ret, sockfd);
    return ret;
}

wrap_define(ssize_t, recvfrom, int sockfd, void *buf, size_t len, 
        int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    ssize_t ret = __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    log_wrap_lib_info("%d = recvfrom(sockfd[%d])", (int)ret, sockfd);
    return ret;
}

wrap_define(ssize_t, recvmsg, int sockfd, struct msghdr *msg, int flags)
{
    ssize_t ret = __real_recvmsg(sockfd, msg, flags);
    log_wrap_lib_info("%d = recvmsg(sockfd[%d], msg[%p], flags[%d])", (int)ret, sockfd, msg, flags);
    return ret;
}

wrap_define(ssize_t, sendto, int socket, const void *message, size_t length, int flags, 
        const struct sockaddr *dest_addr, socklen_t dest_len)
{
    ssize_t ret = __real_sendto(socket, message, length, flags, dest_addr, dest_len);
    log_wrap_lib_info("%d = sendto(socket[%d])", (int)ret, socket);
    return ret;
}

#else

#define socket(domain, type, protocol)      __real_socket(domain, type, protocol)
#define bind(sockfd, addr, addrlen)         __real_bind(sockfd, addr, addrlen)
#define listen(sockfd, backlog)             __real_listen(sockfd, backlog)
#define connect(socket, address, address_len)    __real_connect(socket, address, address_len)
#define send(socket, buffer, length, flags) __real_send(socket, buffer, length, flags)
#define recv(socket, buffer, length, flags) __real_recv(socket, buffer, length, flags)
#define recvfrom(socket, buffer, length, flags, src_addr, addrlen)  __real_recvfrom(socket, buffer, length, flags, src_addr, addrlen)
#define recvmsg(socket, message, flags)     __real_recvmsg(socket, message, flags)
#define sendto(socket, message, length, flags, dest_addr, dest_len)  __real_sendto(socket, message, length, flags, dest_addr, dest_len)

#endif

#ifdef __cplusplus
}
#endif
