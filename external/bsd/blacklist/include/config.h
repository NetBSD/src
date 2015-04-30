#include <stdint.h>
#include <stdio.h>
FILE *popenve(const char *, char *const *, char *const *, const char *);
int pcloseve(FILE *);
#define pclose(a) pcloseve(a);
intmax_t strtoi(const char *, char **, int, intmax_t, intmax_t, int *);
