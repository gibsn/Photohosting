#include <stdio.h>

#include "common.h"
#include "log.h"


int main(int argc, char **argv)
{
    Config cfg;

    if (!process_cmd_arguments(argc, argv, cfg)) return -1;
    cfg.Check();

    return 0;
}
