#ifndef _DEBUGGING_H_
#define _DEBUGGING_H_

#include "common.h"

int countopenfds(void);
int issignalblocked(int signo);

#ifdef DEBUGPRINT
    #include <stdio.h>

    #define DEBUG_COLOR_PRINT(color_code, fmt, ...) \
        fprintf(stderr, \
                "\x1b[" color_code "m" \
				"[%s:%d:%s] " \
				fmt \
				"\x1b[0m" "\n", \
				__FILE__, __LINE__, __func__, ##__VA_ARGS__)

	#define DEBUG_PRINT(fmt, ...) \
		DEBUG_COLOR_PRINT("0", fmt, ##__VA_ARGS__)

    #define LOG_NEWLINE fprintf(stderr, "\n")
    #define FLUSH_LOG fflush(stderr);
    #define LOG_INFO(fmt, ...) DEBUG_COLOR_PRINT(CYAN_COLOR_CODE, "INFO: " fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) DEBUG_COLOR_PRINT(RED_COLOR_CODE, "ERROR: " fmt, ##__VA_ARGS__)

    #define LOG_BAD_ARGS() LOG_ERROR("Bad function arguments.")

    #define LOG_OPEN_FDS_COUNT() LOG_INFO("Open file descriptors count: %d.", countopenfds())
    #define LOG_EXPR_INT(expr) LOG_INFO(#expr " = %d", expr)
#else
    #define DEBUG_PRINT(...) NOP
    #define LOG_NEWLINE NOP
    #define FLUSH_LOG NOP
    #define LOG_INFO(...) NOP
    #define LOG_ERROR(...) NOP

    #define LOG_BAD_ARGS(...) NOP

    #define LOG_OPEN_FDS_COUNT() NOP
    #define LOG_EXPR_INT(expr) NOP
#endif

#ifdef DEBUG
    #include <signal.h>
    #define BREAKPOINT raise(SIGINT)
    #define BREAKPOINT_IF(expr) (expr) ? (BREAKPOINT) : 0
#else
    #define BREAKPOINT NOP
    #define BREAKPOINT_IF(expr) NOP
#endif

#endif /* !_DEBUGGING_H_ */
