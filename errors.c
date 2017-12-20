#include <stdio.h>
#include "chatter.h"

/*
The structure for the management of errors was inspired by this SO post, asked by me:
https://stackoverflow.com/questions/47880223/how-to-programmatically-map-integers-to-const-strings
*/

const error_message_t error_messages[] = {
    { .code = 0,    .msg = "SORRY"},
    { .code = 1,    .msg = "USER %s EXISTS"},
    { .code = 2,    .msg = "%s DOES NOT EXIST"},
    { .code = 10,   .msg = "ROOM EXISTS"},
    { .code = 11,   .msg = "MAXIMUM ROOMS REACHED"},
    { .code = 20,   .msg = "ROOM %s DOES NOT EXIST"},
    { .code = 30,   .msg = "USER NOT PRESENT"},
    { .code = 40,   .msg = "NOT OWNER"},
    { .code = 41,   .msg = "INVALID USER"},
    { .code = 60,   .msg = "INVALID OPERATION"},
    { .code = 61,   .msg = "INVALID PASSWORD"},
    { .code = 62,   .msg = "INVALID FORMAT/NO CARRIAGE RETURN"},
    { .code = 100,  .msg = "INTERNAL SERVER ERROR"}
};

const size_t num_errors = (sizeof(error_messages) / sizeof(error_messages[0]));

const char * get_error(int code)
{

    if (code < 0){
        code = -code;
    }

    for (int i = 0; i < num_errors; i++){
        if (error_messages[i].code == code)
            return error_messages[i].msg;
    }
    error("Unrecognized error code: %d", code);
    return NULL;
}