/* Configuration for GCC for ARM running NetBSD as host.  */

#include <arm32/xm-arm32.h>

/* xm-netbsd.h defines this */
#ifdef HAVE_VPRINTF
#undef HAVE_VPRINTF
#endif

#include <xm-netbsd.h>
