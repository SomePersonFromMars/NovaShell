#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"

#include "executor.h"

static pipelineseq * parsed_line;
static char input_buffer[MAX_LINE_LENGTH+1]; // '\0' sentinel byte at the end
static ssize_t bytes_read;
static bool interactive_mode = true;

void
printprompt(void)
{
    if (!interactive_mode) return;
    fputs(PROMPT_STR, stdout);
    fflush(stdout);
}

ssize_t
readavailableormaxbuff(size_t offset)
{
    errno = 0;
    return bytes_read = read(STDIN_FILENO, input_buffer+offset, MAX_LINE_LENGTH-offset);
}

size_t
findlineend(size_t offset)
{
    return strchrnul(input_buffer+offset, '\n') - input_buffer;
}

void
inputsetup(void)
{
    struct stat statbuf;
    fstat(STDIN_FILENO, &statbuf);
    interactive_mode = S_ISCHR(statbuf.st_mode); // Alternative is to use isatty()
}

#ifdef DEBUG
    int breakpoint_cnter_1 = 0;
#endif

void
inputloop(void)
{
    size_t line_start = 0;
    size_t next_read_offset = 0; // line_start may be smaller than next_read_offset when line prefix move was performed.
    bool ignore_next_line = false;
    while (
        ignore_next_line ? NOP : printprompt(),
        readavailableormaxbuff(next_read_offset) > 0)
    {	
        LOG_INFO("breakpoint_cnter_1 = %d", breakpoint_cnter_1++);

        const size_t applied_read_offset = next_read_offset;
        next_read_offset = 0;
        const size_t input_size = applied_read_offset + bytes_read;
        input_buffer[input_size] = '\0';

        #ifdef DEBUGPRINT
            const int MAX_LENGTH = 80;
            const int SIDE_LENGTH = MAX_LENGTH / 2;
            if (input_size > MAX_LENGTH) {
                LOG_INFO("applied_read_offset = %zu, input_buffer = [%.*s \n---------------------\n %.*s]", applied_read_offset, SIDE_LENGTH, input_buffer, SIDE_LENGTH, &input_buffer[input_size-SIDE_LENGTH]);
            } else {
                LOG_INFO("applied_read_offset = %zu, input_buffer = [%s]", applied_read_offset, input_buffer);
            }
        #endif

        size_t line_end = applied_read_offset; // Line end search starts here
        while (line_start < input_size) { // Line should be at least 1 character
            line_end = findlineend(line_end);

            if (line_end >= MAX_LINE_LENGTH) {
                const size_t line_len = (MAX_LINE_LENGTH-1) - (line_start-1);
                if (line_len < MAX_LINE_LENGTH) {
                    assert(line_start > 0);
                    memmove(input_buffer, input_buffer+line_start, line_len);

                    next_read_offset = line_len;
                    line_start = 0;
                    break;
                } else {
                    assert(line_start == 0);
                    if (!ignore_next_line) {
                        fputs(SYNTAX_ERROR_STR "\n", stderr);
                        LOG_ERROR("Input line is too long. line_start = %zu, line_end = %zu. It's longer than %d characters.", line_start, line_end, MAX_LINE_LENGTH);
                        fflush(stderr);
                    }

                    ignore_next_line = true;
                    next_read_offset = 0;
                    break;
                }
            } else if (line_end >= input_size) {
                const size_t line_len = (input_size-1) - (line_start-1);

                next_read_offset = line_end;
                break;
            }

            if (ignore_next_line) {
                ignore_next_line = false;
            } else {
                input_buffer[line_end] = '\0';
                LOG_INFO("Found line: [%s]", &input_buffer[line_start]);
                parsed_line = parseline(&input_buffer[line_start]);
                #ifdef DEBUGPRINT
                    // printparsedline(parsed_line);
                #endif
                command *com = pickfirstcommand(parsed_line);
                executecommand(com);
            }

            line_start = line_end+1;
            line_end = line_start;
        }
        if (line_start >= input_size)
            line_start = 0;
    }

    if (bytes_read < 0) {
        LOG_ERROR("read() set errno = %d.", errno);
        exit(EXIT_FAILURE);
    }
}
