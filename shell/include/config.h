#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_LINE_LENGTH 2048
#define DEFAULT_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

#define SYNTAX_ERROR_STR "Syntax error."

#define BUILTIN_FAILURE 1
#define EXEC_FAILURE 127

#define PROMPT_FMT_STR ( \
        "\x1b[" BLUE_COLOR_CODE "m" \
        "[NovaShell]" \
        "\x1b[0m" \
        \
        "\x1b[" GRAY_COLOR_CODE "m" \
        " %s" \
        "\x1b[0m" \
        \
        "$ " \
        )

#endif /* !_CONFIG_H_ */
