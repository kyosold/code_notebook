#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include "uniqid.h"

/**
 * @brief  Generate a unique ID
 * Gets a prefixed unique identifier based on the current time in microseconds.
 * @param prefix Can be useful, for instance, if you generate identifiers simultaneously on several hosts that might happen to generate the identifier at the same microsecond. With an empty prefix, the returned string will be 13 characters long
 * @param uid Returns timestamp based unique identifier as a string.
 * @param uid_size result memory size
 * @return 
 */
void s_uniqid(char *prefix, char *uid, size_t uid_size)
{
    int sec, usec, prefix_len = 0;
    struct timeval tv;

    gettimeofday((struct timeval *)&tv, (struct timezone *)NULL);
    sec = (int)tv.tv_sec;
    usec = (int)(tv.tv_usec % 0x100000);

    /**
     * The max value usec can have is 0xF423F, so we use only five hex
     * digits for usecs.
     */
    snprintf(uid, uid_size, "%s%08x%05x", prefix, sec, usec);
}

#ifdef _TEST
// gcc -g uniqid.c -D_TEST
int main(int argc, char **argv)
{
    char uid[1024] = {0};
    s_uniqid("", uid, sizeof(uid));
    printf("uid: %s\n", uid);
    return 0;
}

#endif