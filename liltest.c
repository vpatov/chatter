#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include <string.h>

int
main(int argc, char *argv[])
{



    char *message = "IAMNEW vasia \r\n";
    char *ptr;
    ptr = strstr(message,"NEW");

    info("%x",ptr);
    info("%x",message);

    int x;
    x = 5;

}