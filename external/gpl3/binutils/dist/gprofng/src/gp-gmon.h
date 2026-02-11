#ifndef gp_gmon_h
#define gp_gmon_h

#include "corefile.h"

#ifdef DEBUG
#define DBG(l,s) {s;}
#else
#define DBG(l,s)
#endif

#define done(INT) exit(INT)

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#endif /* gp_gmon_h */
