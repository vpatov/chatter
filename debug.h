#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define NL "\n"

#define KNRM "\033[0m"
#define KRED "\033[1;31m"
#define KGRN "\033[1;32m"
#define KYEL "\033[1;33m"
#define KBLU "\033[1;34m"
#define KMAG "\033[1;35m"
#define KCYN "\033[1;36m"
#define KWHT "\033[1;37m"
#define KBWN "\033[0;33m"

static inline unsigned int thread_id() {
    return (unsigned long)pthread_self();
}

#ifndef DEBUG
#define PRINTSTMNT(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stderr, level ": " KNRM fmt NL, thread_id(), ##__VA_ARGS__);                                                        \
    } while (0)
#else
#define PRINTSTMNT(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(                                                                                                       \
            stderr, level ": %s:%s:%d " KNRM fmt NL, thread_id(), _FILE_, _extension_ _FUNCTION_, _LINE_, ##__VA_ARGS__);   \
    } while (0)
#endif

#define debug(S, ...) PRINTSTMNT(KMAG "%02X | DEBUG", S, ##__VA_ARGS__)
#define info(S, ...) PRINTSTMNT(KBLU "%02X | INFO", S, ##__VA_ARGS__)
#define warn(S, ...) PRINTSTMNT(KYEL "%02X | WARN", S, ##__VA_ARGS__)
#define success(S, ...) PRINTSTMNT(KGRN "%02X | SUCCESS", S, ##__VA_ARGS__)
#define error(S, ...) PRINTSTMNT(KRED "%02X | ERROR", S, ##__VA_ARGS__)

#endif /* DEBUG_H */