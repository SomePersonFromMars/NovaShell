#include "inputhandler.h"
#include "subprocessesmanager.h"

// TODO research BSD naming convention
// TODO pick naming conventions

int
main(int argc, char *argv[])
{
    setupsubprocesseswatcherandsignals();
    inputsetup();
    inputloop();
    return 0;
}
