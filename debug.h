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

#define PRERR "\x1B[38;2;210;60;60m" "\x1B[48;2;5;5;5m"

#define INFO_COLOR 		"\x1B[1;34m"
#define ERROR_COLOR		"\x1B[1;31m"
#define PRIVATE_COLOR	"\x1B[1;35m"	
#define DEFAULT_COLOR	"\x1B[0m"	


#define SEND_FG "\x1B[38;2;90;90;255m" 
#define SEND_BG "\x1B[48;2;10;10;10m"

#define ECHO_FG "\x1B[38;2;144;178;206m"
#define PRIV_FG KMAG
#define ROOM_FG KYEL

#define RECV_FG "\x1B[38;2;90;255;90m"
#define RECV_BG "\x1B[48;2;10;10;10m"

#define GRAY    "\x1B[38;2;210;210;210m"




#define i_1e3 1000
#define i_1e6 1000000
#define i_1e9 1000000000

static inline unsigned int thread_id() {
    return (unsigned long)pthread_self();
}

#ifndef DEBUG
#define PRINTSTMNT(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stderr, level   fmt KNRM NL, thread_id(), ##__VA_ARGS__);                                                        \
    } while (0)
#else
#define PRINTSTMNT(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(                                                                                                       \
            stderr, level "%s:%s:%d " KNRM fmt NL, thread_id(), _FILE_, _extension_ _FUNCTION_, _LINE_, ##__VA_ARGS__);   \
    } while (0)
#endif


//print out without thread id.
#define PRINTSTMNTW(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stderr, level   fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)


//print out sendecho
#define PRINTSTMNTS(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, level  SEND_BG fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)


//print out echo
#define PRINTSTMNTECHO(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, level fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)


//print out private message
#define PRINTSTMNTPRIV(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, level fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)

//print out room echo message
#define PRINTSTMNTECHOR(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, level fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)

//print out recvecho
#define PRINTSTMNTR(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, level  RECV_BG fmt KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)

#define PRINTSTMNTP(fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, KCYN fmt  KNRM NL, ##__VA_ARGS__);                                                        \
    } while (0)


#define PRINTSTMNTE(fmt, ...)                                                                                    \
    do {                                                                                                               \
        fprintf(stdout, PRERR fmt  KNRM NL , ##__VA_ARGS__);                                                        \
    } while (0)



#define debug(S, ...) PRINTSTMNT(KMAG 						"%08X | DEBUG   :  ", S, ##__VA_ARGS__)
#define info(S, ...) PRINTSTMNT(KBLU 						"%08X | INFO    :  ", S, ##__VA_ARGS__)
#define warn(S, ...) PRINTSTMNT(KYEL 						"%08X | WARN    :  ", S, ##__VA_ARGS__)
#define success(S, ...) PRINTSTMNT(KGRN 					"%08X | SUCCESS :  ", S, ##__VA_ARGS__)
#define error(S, ...) PRINTSTMNT(KRED 						"%08X | ERROR   :  ", S, ##__VA_ARGS__)



#define debugw(S, ...) PRINTSTMNTW(KMAG 				"DEBUG              :  ", S, ##__VA_ARGS__)
#define infow(S, ...) PRINTSTMNTW(KBLU  				"INFO               :  ", S, ##__VA_ARGS__)
#define warnw(S, ...) PRINTSTMNTW(KYEL 					"WARN               :  ", S, ##__VA_ARGS__)
#define successw(S, ...) PRINTSTMNTW(KGRN 				"SUCCESS            :  ", S, ##__VA_ARGS__)
#define errorw(S, ...) PRINTSTMNTW(KRED 				"ERROR              :  ", S, ##__VA_ARGS__)

#define sendecho(S, ...) PRINTSTMNTS(SEND_FG 			"ECHO (Sending)     :  ", S, ##__VA_ARGS__)
#define recvecho(S, ...) PRINTSTMNTR(RECV_FG   			"ECHO (Receiving)   :  ", S, ##__VA_ARGS__)

#define errorp(S, ...) PRINTSTMNTE(S, ##__VA_ARGS__)

#define promptnl(S, ...) PRINTSTMNTP(S, ##__VA_ARGS__)
#define prompt(S, ...) PRINTSTMNTP(S, ##__VA_ARGS__)

#define printecho(S, ...) PRINTSTMNTECHO(ECHO_FG        "ECHO server        :  ", S, ##__VA_ARGS__)
#define printpriv(S, ...) PRINTSTMNTPRIV(PRIV_FG  "ECHOP ",  S, ##__VA_ARGS__)
#define printechor(S, ...) PRINTSTMNTECHOR(ROOM_FG  "ECHO ",  S, ##__VA_ARGS__)





#endif /* DEBUG_H */