#ifndef __TASK_H__
#define __TASK_H__

int task_start(int (*func)(int , char **), int argc, char **argv);

#endif
