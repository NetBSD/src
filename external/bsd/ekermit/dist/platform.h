/* Unix platform.h for EK */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#undef X_OK	/* namespace collision between kermit.h and unistd.h */

#ifndef IBUFLEN
#define IBUFLEN  4096			/* File input buffer size */
#endif /* IBUFLEN */

#ifndef OBUFLEN
#define OBUFLEN  8192                   /* File output buffer size */
#endif /* OBUFLEN */
