#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("%d\n", getpid());
    fflush(stdout);
    return 0;
}
