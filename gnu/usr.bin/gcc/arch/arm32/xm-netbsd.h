/* Configuration for GCC for ARM running NetBSD as host.  */

#define GCC_27_ARM32_PIC_SUPPORT

#include <arm32/xm-arm32.h>

/* xm-netbsd.h defines this */
#ifdef HAVE_VPRINTF
#undef HAVE_VPRINTF
#endif

#include <xm-netbsd.h>
