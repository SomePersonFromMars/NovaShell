#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "common.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"

#include "executor.h"

static pipelineseq * parsed_line;
static char input_line[MAX_LINE_LENGTH];
static ssize_t bytes_read;

void
printprompt(void)
{
    fputs(PROMPT_STR, stdout);
    fflush(stdout);
}

ssize_t
readlineormaxbuff(void)
{ return bytes_read = read(STDIN_FILENO, input_line, MAX_LINE_LENGTH); }

bool
handletoolonginput(void)
{
    if (input_line[bytes_read-1] != '\n') {
        while (input_line[bytes_read-1] != '\n')
            readlineormaxbuff();
        fputs(SYNTAX_ERROR_STR "\n", stderr);
        LOG_ERROR(
            "Input line is too long. It's longer than %d characters.",
            MAX_LINE_LENGTH);
        return true;
    }
    return false;
}

bool
handleparsingerrors(void)
{
    if (parsed_line == NULL) {
        fputs(SYNTAX_ERROR_STR "\n", stderr);
        LOG_ERROR(
            "Parsing error.");
        return true;
    }
    return false;
}

void
inputloop(void)
{
    while (
        printprompt(),
        errno = 0,
        readlineormaxbuff() > 0)
    {	
        input_line[bytes_read] = '\0';
        if (handletoolonginput()) continue;
        parsed_line = parseline(input_line);
        if (handleparsingerrors()) continue;

        #ifdef DEBUG
            printparsedline(parsed_line);
        #endif

        command *com = pickfirstcommand(parsed_line);
        executecommand(com);
    }

    if (bytes_read < 0) {
        LOG_ERROR("read() set errno = %d.", errno);
        exit(EXIT_FAILURE);
    }
}
