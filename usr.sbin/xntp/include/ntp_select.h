/*
 * Not all machines define FD_SET in sys/types.h
 */ 
#ifndef _ntp_select_h
#define _ntp_select_h

/* Was: (defined(RS6000)||defined(SYS_PTX))&&!defined(_BSD) */
/* Could say: !defined(FD_SET) && defined(HAVE_SYS_SELECT_H) */
#if defined(HAVE_SYS_SELECT_H) && !defined(_BSD)
#include <sys/select.h>
#endif

#if !defined(FD_SET) && !defined(SYS_WINNT)
#define NFDBITS         32
#define FD_SETSIZE      32
#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      memset((char *)(p), 0, sizeof(*(p)))
#endif

#if defined(VMS)
typedef struct {
    unsigned int fds_bits[1];
} fd_set;
#endif

#endif /* _ntp_select_h */
