#ifndef COMPAT_H_
#define COMPAT_H_

#include "config.h"

#include <sys/types.h>

#define ISCSI_HTON16(a)	htons(a)
#define ISCSI_HTON32(a)	htonl(a)
#define ISCSI_HTON64(a)	htonq(a)
#define ISCSI_NTOH16(a)	ntohs(a)
#define ISCSI_NTOH32(a)	ntohl(a)
#define ISCSI_NTOH64(a)	ntohq(a)

u_int64_t htonq(u_int64_t);
u_int64_t ntohq(u_int64_t);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#endif
