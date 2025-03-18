#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

void func3()
{
    void *ret = malloc(10);
}

void func2()
{
    void *ret = malloc(10);
    func3();
    free(ret);
}

void func1()
{
    void *ret = malloc(10);
    func2();
    free(ret);
}

void func11()
{
    void *ptr = malloc(1);
    void *ptr2 = malloc(1);
    void *ptr3 = malloc(1);
}

int main(int argc, char **argv)
{
    if (strcmp(argv[1], argv[2]) == 0) {}
    
    printf("%d\n", getpid());
    func1();
    int fd = open("./Makefile", O_RDONLY);
    FILE *fp = popen("ls -l", "r");
    pclose(fp);
    if (strcmp(argv[1], argv[2]) == 0) {}
    if (fork() == 0) {
        printf("%d\n", getpid());
        func1();
    if (strcmp(argv[1], argv[2]) == 0) {}
    close(fd);
    }
    else {
    close(fd);
    }
    system("ls -l");
    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *))func11, NULL);
    return 0;
}
