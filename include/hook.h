#ifndef __HOOK_H_
#define __HOOK_H_

#if 0
#ifdef HOOK_OPEN
typedef int (*OPEN)(const char *, int, ...); 

extern OPEN open_orign;

#ifdef open
#undef open
#endif

#define open open_orign 
#endif
#endif

#endif
