#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"

#include "executor.h"

// TODO research BSD naming convention
// TODO pick naming conventions

void
skipinputline(void)
{
    char next_char;
    ssize_t bytes_read;
    while (
        (bytes_read = read(STDIN_FILENO, &next_char, 1)) > 0 &&
        next_char != '\n')
    { }
}

int
main(int argc, char *argv[])
{
    pipelineseq * parsed_line;

    char input_line[MAX_LINE_LENGTH];

    ssize_t bytes_read;
    while (
            fputs(PROMPT_STR, stdout),
            fflush(stdout),
            errno = 0,
            (bytes_read = read(STDIN_FILENO, input_line, MAX_LINE_LENGTH)) > 0)
    {	
        input_line[bytes_read] = '\0';
        if (input_line[bytes_read-1] != '\n') {
            fputs(SYNTAX_ERROR_STR "\n", stderr);
            LOG_ERROR(
                "Input line is too long. It's longer than %d characters.",
                MAX_LINE_LENGTH);
            skipinputline();
            continue;
        }

        parsed_line = parseline(input_line);
        if (parsed_line == NULL) {
            fputs(SYNTAX_ERROR_STR "\n", stderr);
        }
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

    return 0;
}
