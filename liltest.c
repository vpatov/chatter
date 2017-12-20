#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include <string.h>




int
main(int argc, char *argv[])
{

    char message[1024];
    strcpy(message,"IAMNEW vasia \r\nBYE mofo \r\n");

    int tokens = tokenize(message, " \r\n");

    info("tokens: %d", tokens);

    info("token1: %s", get_token(message," \r\n",0));
    info("token1: %s", get_token(message," \r\n",1));
    info("token2: %s", get_token(message," \r\n",2));

}