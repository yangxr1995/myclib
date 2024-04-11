#ifndef __TASK_H__
#define __TASK_H__

int task_start(int (*func)(void *), void *arg, const char *prgname);
int task_start_main(int (*func)(int, char **), int argc, char **argv);

#endif
