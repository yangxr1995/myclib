#include <sys/types.h>
#include <libgen.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

pid_t child_pid = -1;
char pid_filename[256];

void
do_exit()
{
	unlink(pid_filename);
	if (child_pid > 0) {
		kill(child_pid, SIGTERM);
		log_message(DEBUG_LOG, "%s : kill %d", __func__, child_pid);
	}
}

void
do_signal(int sig)
{
	log_message(INFO_LOG, "%s : %d", __func__, sig);
	switch (sig) {
		case SIGKILL:
		case SIGINT:
		case SIGABRT:
		case SIGQUIT:
		case SIGSEGV:
		case SIGTERM:
			do_exit();
			exit(0);
			break;
	}
}

int
save_pid(const char *prgname)
{
	pid_t pid;	
	FILE *fp = NULL;

	pid = getpid();

	snprintf(pid_filename, sizeof(pid_filename), "/var/run/%s.pid", prgname);

	if ((fp = fopen(pid_filename, "w")) == NULL) {
		log_message(ERR_LOG, "%s : save_pid : %s", __func__, strerror(errno));
		goto err;
	}

	fprintf(fp, "%d", pid);	

	if (fp)
		fclose(fp);

	return 0;

err:
	if (fp)
		fclose(fp);
	return -1;
}

int
task_start_main(int (*func)(int, char **), int argc, char **argv)
{
	int ret, status;
	pid_t pid;

	daemon(0, 0);

	signal(SIGINT, do_signal);
	signal(SIGABRT, do_signal);
	signal(SIGKILL, do_signal);
	signal(SIGQUIT, do_signal);
	signal(SIGSEGV, do_signal);
	signal(SIGTERM, do_signal);

	save_pid(basename(argv[0]));

	while (1) {
		if ((pid = fork()) < 0) {
			log_message(ERR_LOG, "%s : fork : %s", __func__, strerror(errno));
			return -1;
		}

		if (pid == 0) {
			signal(SIGINT, SIG_DFL);
			signal(SIGABRT, SIG_DFL);
			signal(SIGKILL, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGSEGV, SIG_DFL);
			signal(SIGTERM, SIG_DFL);

			ret = func(argc, argv);
			exit(ret);
		}
			
		child_pid = pid;
		if (waitpid(pid, &status, 0) < 0) {
			log_message(ERR_LOG, "%s : waitpid : %s", __func__, strerror(errno));
			goto err;
		}

		if (!WIFEXITED(status)) {
			if (WIFSIGNALED(status))
				log_message(INFO_LOG, "child exit by signal '%d'", WTERMSIG(status));
		}
		else {
			if (WEXITSTATUS(status) == 0) {
				child_pid = -1;
				break;	
			}

			log_message(INFO_LOG, "child exit '%d'", WEXITSTATUS(status));
		}		

		log_message(INFO_LOG, "restart child");
		sleep(1);
	}

	do_exit();
	exit(0);

err:
	do_exit();
	exit(-1);
}

int
task_start(int (*func)(void *), void *arg, const char *prgname)
{
	int ret, status;
	pid_t pid;

	daemon(0, 0);

	signal(SIGINT, do_signal);
	signal(SIGABRT, do_signal);
	signal(SIGKILL, do_signal);
	signal(SIGQUIT, do_signal);
	signal(SIGSEGV, do_signal);
	signal(SIGTERM, do_signal);

	save_pid(prgname);

	while (1) {
		if ((pid = fork()) < 0) {
			log_message(ERR_LOG, "%s : fork : %s", __func__, strerror(errno));
			return -1;
		}

		if (pid == 0) {
			signal(SIGINT, SIG_DFL);
			signal(SIGABRT, SIG_DFL);
			signal(SIGKILL, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGSEGV, SIG_DFL);
			signal(SIGTERM, SIG_DFL);

			ret = func(arg);
			exit(ret);
		}
			
		child_pid = pid;
		if (waitpid(pid, &status, 0) < 0) {
			log_message(ERR_LOG, "%s : waitpid : %s", __func__, strerror(errno));
			goto err;
		}

		if (!WIFEXITED(status)) {
			if (WIFSIGNALED(status))
				log_message(INFO_LOG, "child exit by signal '%d'", WTERMSIG(status));
		}
		else {
			if (WEXITSTATUS(status) == 0) {
				child_pid = -1;
				break;	
			}

			log_message(INFO_LOG, "child exit '%d'", WEXITSTATUS(status));
		}		

		log_message(INFO_LOG, "restart child");
		sleep(1);
	}

	do_exit();
	exit(0);

err:
	do_exit();
	exit(-1);
}
