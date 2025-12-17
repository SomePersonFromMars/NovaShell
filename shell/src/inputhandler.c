#include <linux/limits.h>
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
#include "debugging.h"
#include "config.h"
#include "siparse.h"

#include "subprocessesmanager.h"
#include "executor.h"

static pipelineseq * parsed_line;
// '\0' sentinel byte at the end
static char input_buffer[MAX_LINE_LENGTH+1];
static bool interactive_mode = true;

static size_t line_start, line_end;
static size_t applied_read_offset;
static size_t next_read_offset;
static size_t input_size;
static ssize_t bytes_read;
bool ignore_next_line;

static char path_buffer[PATH_MAX];

// May return a pointer to statically allocated buffer.
static char*
getnicecwd(void)
{
    char * const original_path = get_current_dir_name();
    char * const home_path = secure_getenv("HOME");
    assert(original_path);
    const size_t home_path_len = (home_path ? strlen(home_path) : 0);
    const size_t original_path_len = strlen(original_path);
    if (home_path && strncmp(original_path, home_path, home_path_len) == 0) {
        const size_t rest_len = original_path_len - home_path_len;
        path_buffer[0]='~';
        memcpy(path_buffer + 1, original_path + home_path_len, rest_len + 1);
        return path_buffer;
    } else {
        return original_path;
    }
}

void
printprompt(void)
{
    if (!interactive_mode) return;
    printpendingbgchildrenstatuses();
    printf(PROMPT_FMT_STR, getnicecwd());
    fflush(stdout);
}

ssize_t
readavailableormaxbuff(size_t offset)
{
    errno = 0;
    return bytes_read = read(
        STDIN_FILENO,
        input_buffer+offset,
        MAX_LINE_LENGTH-offset
    );
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
    // Alternative is to use isatty()
    interactive_mode = S_ISCHR(statbuf.st_mode);
}

static void
logparserinfo(void)
{
    const int MAX_LENGTH = 80;
    const int SIDE_LENGTH = MAX_LENGTH / 2;
    if (input_size > MAX_LENGTH) {
        LOG_INFO(
            "applied_read_offset = %zu,"
            "input_buffer = [%.*s \n---------------------\n %.*s]",
            applied_read_offset, SIDE_LENGTH,
            input_buffer, SIDE_LENGTH,
            &input_buffer[input_size-SIDE_LENGTH]
        );
    } else {
        LOG_INFO(
            "applied_read_offset = %zu, input_buffer = [%s]",
            applied_read_offset, input_buffer
        );
    }
}

static void
movelinetobufferbeg(const size_t line_len)
{
    assert(line_start > 0);
    memmove(input_buffer, input_buffer+line_start, line_len);

    next_read_offset = line_len;
    line_start = 0;
}

static void
linetoolong(void)
{
    assert(line_start == 0);
    if (!ignore_next_line) {
        fputs(SYNTAX_ERROR_STR "\n", stderr);
        LOG_ERROR(
            "Input line is too long."
            "line_start = %zu, line_end = %zu."
            "It's longer than %d characters.",
            line_start, line_end, MAX_LINE_LENGTH
        );
        FLUSH_LOG;
    }

    ignore_next_line = true;
    next_read_offset = 0;
}

static void
parseandexecuteline(void)
{
    input_buffer[line_end] = '\0';
    LOG_INFO("Found line: [%s]", &input_buffer[line_start]);
    parsed_line = parseline(&input_buffer[line_start]);
    #ifdef DEBUGPRINT
        printparsedline(parsed_line);
    #endif
    LOG_OPEN_FDS_COUNT();
    executeline(parsed_line);
}

static void
parseinputbuffer(void)
{
    applied_read_offset = next_read_offset;
    next_read_offset = 0;
    input_size = applied_read_offset + bytes_read;
    input_buffer[input_size] = '\0';

    logparserinfo();

    // Line end search starts here
    line_end = applied_read_offset;

    // Line should be at least 1 character
    while (line_start < input_size) {
        line_end = findlineend(line_end);

        if (line_end >= MAX_LINE_LENGTH) {
            const size_t line_len
                = (MAX_LINE_LENGTH-1) - (line_start-1);
            if (line_len < MAX_LINE_LENGTH)
                movelinetobufferbeg(line_len);
            else
                linetoolong();
            break;
        } else if (line_end >= input_size) {
            const size_t line_len = (input_size-1) - (line_start-1);
            next_read_offset = line_end;
            break;
        }

        if (ignore_next_line) {
            ignore_next_line = false;
        } else {
            parseandexecuteline();
        }

        line_start = line_end+1;
        line_end = line_start;
    }
    if (line_start >= input_size)
        line_start = 0;
}

void
inputloop(void)
{
    line_start = 0;
    // line_start may be smaller than
    // next_read_offset when line prefix move was performed.
    next_read_offset = 0;
    ignore_next_line = false;
    while (
        ignore_next_line ? NOP : printprompt(),
        readavailableormaxbuff(next_read_offset) > 0
    ) {	
        parseinputbuffer();
    }
    if (interactive_mode)
        fputs("\n", stdout);

    if (bytes_read < 0) {
        LOG_ERROR("read() set errno = %d.", errno);
        exit(EXIT_FAILURE);
    }
}
