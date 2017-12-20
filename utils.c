#include "chatter.h"

//taken from https://stackoverflow.com/a/7775170/3664123
void strip_char(char *str, char strip)
{
    char *p, *q;
    for (q = p = str; *p; p++){
        if (*p != strip)
            *q++ = *p;
    }
    *q = '\0';
}


unsigned int randint(){
	unsigned int rand;
	if (!RAND_bytes((unsigned char*)&rand, sizeof(unsigned int))){
		error("RAND_bytes returned an error.");
	}
	return rand;
}


char*
inet4_ntop(char *dst, unsigned int addr)
{
    snprintf(dst,INET_ADDRSTRLEN,"%d.%d.%d.%d\n", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF); 
    return dst;
}