#include "argsparser.h"

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

// Converts a given str to long only if the whole str
// is a valid decimal number. Returns false on failure.
bool
strtolong(char * str, long *out)
{
    char *str_end;
    errno = 0;
    long value = strtol(str, &str_end, 10);
    if (errno == EINVAL || errno == ERANGE || str_end == str || *str_end != '\0')
        return false;
    *out = value;
    return true;
}

// Converts a given str to int only if the whole str
// is a valid decimal number. Returns false on failure.
bool
strtoint(char * str, int *out)
{
    long value_long;
    if (!strtolong(str, &value_long)) return false;
    if (value_long < INT_MIN || value_long > INT_MAX) return false;
    *out = value_long;
    return true;
}
