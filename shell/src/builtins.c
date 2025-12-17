#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "debugging.h"
#include "argsparser.h"
#include "config.h"
#include "builtins.h"

int exitbuiltin(char *[]);
int echo(char*[]);
int cd(char*[]);
int killbuiltin(char*[]);
int ls(char*[]);
int undefined(char *[]);

builtin_pair builtins_table[]={
	{"exit",	&exitbuiltin},
	{"lecho",	&echo},
	{"lcd",		&cd},
	{"cd",		&cd},
	{"lkill",	&killbuiltin},
	{"lls",		&ls},
	{NULL,NULL}
};

builtin_pair*
getbuiltincommand(const char * name)
{
    builtin_pair *ptr = builtins_table;
    while (ptr->name) {
        if (strcmp(name, ptr->name) == 0) {
            return ptr;
        }
        ++ptr;
    }
    return NULL;
}

void
builtinerror(const char * command_name)
{
    fprintf(stderr, "Builtin %s error.\n", command_name);
    fflush(stderr);
}

int
exitbuiltin(char *argv[])
{
    int exit_status;
    if (argv[0] == NULL) {
        exit_status = EXIT_FAILURE;
        builtinerror("exit");
    } else if (argv[1] == NULL) {
        exit_status = 0;
    } else {
        if (argv[2] != NULL) {
            exit_status = EXIT_FAILURE;
            builtinerror("exit");
        } else {
            int wanted_status;
            if (!strtoint(argv[1], &wanted_status)) {
                exit_status = EXIT_FAILURE;
                builtinerror("exit");
            } else {
                exit_status = wanted_status;
            }
        }
    }
    exit(exit_status);
}

int 
echo( char * argv[])
{
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int
cd(char *argv[])
{
    #define ERROR_ACTION do { builtinerror("lcd"); return BUILTIN_FAILURE; } while (0)

    char * path;
    if (argv[0] == NULL) ERROR_ACTION;
    else if (argv[1] == NULL) {
        path = secure_getenv("HOME");
        if (path == NULL) ERROR_ACTION;
    } else {
        if (argv[2] != NULL) ERROR_ACTION;
        path = argv[1];
    }

    if (chdir(path) == -1) ERROR_ACTION;

    return 0;
    #undef ERROR_ACTION
}

int
killbuiltin(char *argv[])
{
    #define ERROR_ACTION do { builtinerror("lkill"); return BUILTIN_FAILURE; } while (0)

    int signal_number;
    pid_t pid;

    if (argv[0] == NULL) ERROR_ACTION;
    else if (argv[1] == NULL) ERROR_ACTION;
    else {
        int arg_val;
        if (!strtoint(argv[1], &arg_val)) ERROR_ACTION;

        if (argv[2] != NULL) {
            if (argv[3] != NULL) ERROR_ACTION;

            signal_number = -arg_val;

            if (!strtoint(argv[2], &arg_val)) ERROR_ACTION;
            pid = arg_val;
        } else {
            signal_number = SIGTERM;

            pid = arg_val;
        }
    }

    LOG_INFO("pid = %d, signal_number = %d", pid, signal_number);
    if (kill(pid, signal_number) == -1) ERROR_ACTION;

    #undef ERROR_ACTION
}

int
ls(char *argv[])
{
    #define ERROR_ACTION do { builtinerror("lls"); return BUILTIN_FAILURE; } while (0)

    char * cwd = get_current_dir_name();
    if (cwd == NULL) ERROR_ACTION;

    DIR *dir = opendir(cwd);
    if (dir == NULL) ERROR_ACTION;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.')
            continue;
        puts(entry->d_name);
        fflush(stdout);
    }

    if (closedir(dir) == -1) ERROR_ACTION;

    #undef ERROR_ACTION
}

int 
undefined(char * argv[])
{
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}
