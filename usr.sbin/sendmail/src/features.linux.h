/*
**  This should be renamed features.h on Linux.  DO NOT USE IT
**  on any other system.
*/

#include_next <features.h>
#undef  __USE_POSIX2

#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __USE_BSD
#define __USE_BSD
#endif
