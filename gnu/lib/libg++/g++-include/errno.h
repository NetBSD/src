//	$Id: errno.h,v 1.2 1993/08/02 17:22:06 mycroft Exp $

#ifndef errno_h

extern "C" {

#ifdef __errno_h_recursive
#include_next <errno.h>
#else
#define __errno_h_recursive
#include_next <errno.h>

#define errno_h 1

extern char*    sys_errlist[];
extern int      sys_nerr;
#ifndef errno                  
extern int      errno;
#endif
void      perror(const char*);
char*     strerr(int);

#endif
}

#endif
