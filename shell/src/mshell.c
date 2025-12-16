#include "inputhandler.h"
#include "subprocessesmanager.h"

int
main(int argc, char *argv[])
{
    setupsubprocesseswatcherandsignals();
    inputsetup();
    inputloop();
    return 0;
}
