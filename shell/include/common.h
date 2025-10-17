#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[%s:%d:%s] " fmt "\n", \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) DEBUG_PRINT("INFO: " fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) DEBUG_PRINT("ERROR: " fmt, ##__VA_ARGS__)

    #define LOG_BAD_ARGS() LOG_ERROR("Bad function arguments.")
#else
    #define NOP ((void)0)

    #define DEBUG_PRINT(...) NOP
    #define LOG_INFO(...) NOP
    #define LOG_ERROR(...) NOP

    #define LOG_BAD_ARGS(...) NOP
#endif

#endif /* !_COMMON_H_ */
