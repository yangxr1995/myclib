#include <stdio.h>

typedef struct cmd_s cmd_t;

struct cmd_s {
	char *name;
	char *help;
};

/*
 * __attribute__((used)) : 
 *    汇编时，将修饰符号标记到目标文件, 
 *    确保链接时会合并符号，即使符号未被引用
 *
 * __attribute__((section("xxx"))) :
 *    将修饰符号在指定段分配空间，若不指定，
 *    则随机分配空间
 *    段的起始地址为 &__start_xxx
 *        结束地址为 &__stop_xxx
 */

cmd_t cmd_echo
__attribute__((used)) 
__attribute__((section("cmd"))) = {
	.name = "echo",
	.help = "echo <xxx>\n",
};

cmd_t cmd_cat
__attribute__((used)) 
__attribute__((section("cmd"))) = {
	.name = "cat",
	.help = "cat <xxx>\n",
};

extern cmd_t __start_cmd;
extern cmd_t __stop_cmd;

typedef struct
{
    char  member1;
    int   member2;
    short member3;
}  __attribute__ ((aligned (1)))f1_t; 

f1_t  Family;

typedef struct
{
    char  member1;
    int   member2;
    short member3;
} __attribute__ ((aligned (8))) f2_t;

f2_t Family2;

int cmd_main()
{

	printf("size : %d\n ", sizeof(Family));
	printf("size : %d\n ", sizeof(Family2));

	cmd_t *cmd;	

	for (cmd = &__start_cmd; cmd < &__stop_cmd;
		//cmd = (cmd_t *)((char *)cmd + sizeof(*cmd))) {
		++cmd) {
		printf("name : %s, help : %s\n", cmd->name, cmd->help);	
	}

	return 0;
}
