#include <stdio.h>

typedef struct cmd_s cmd_t;

struct cmd_s {
	char *name;
	char *help;
};


cmd_t cmd_find
__attribute__((used)) 
__attribute__((section("cmd"))) = {
	.name = "find",
	.help = "find <xxx>\n",
};


