/* opensslconf.h */
/* WARNING: Generated automatically from opensslconf.h.in by Configure. */

/* necessary for u_int32_t and others */
#include <sys/types.h>

/* OpenSSL was configured with the following options: */
#ifdef OPENSSL_ALGORITHM_DEFINES
   /* no ciphers excluded */
#endif
#ifdef OPENSSL_THREAD_DEFINES
#endif
#ifdef OPENSSL_OTHER_DEFINES
#endif

/* crypto/opensslconf.h.in */

/* Generate 80386 code? */
#undef I386_ONLY

#if !(defined(VMS) || defined(__VMS)) /* VMS uses logical names instead */
#if defined(HEADER_CRYPTLIB_H) && !defined(OPENSSLDIR)
#define OPENSSLDIR "/usr/local/ssl"
#endif
#endif

#define OPENSSL_UNISTD <unistd.h>
