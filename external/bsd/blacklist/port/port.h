#include <stdio.h>
#include <inttypes.h>

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif
#ifndef __UNCONST
#define __UNCONST(a) ((void *)(intptr_t)(a))
#endif
#ifndef __arraycount
#define __arraycount(a) (sizeof(a) / sizeof(a[0]))
#endif

FILE *popenve(const char *, char *const *, char *const *, const char *);
int pcloseve(FILE *);
struct sockaddr;
int sockaddr_snprintf(char *, size_t, const char *, const struct sockaddr *);
intmax_t strtoi(const char *, char **, int, intmax_t, intmax_t, int *);
struct timespec;
int clock_gettime(int, struct timespec *);
#define CLOCK_REALTIME 0
