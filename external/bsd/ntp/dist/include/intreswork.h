/*	$NetBSD: intreswork.h,v 1.2.6.2 2014/12/25 02:34:32 snj Exp $	*/

/*
 * intreswork.h -- declarations private to ntp_intres.c, ntp_worker.c.
 */
#ifndef INTRESWORK_H
#define INTRESWORK_H

#include "ntp_worker.h"

#ifdef WORKER

extern int blocking_getaddrinfo(blocking_child *,
				blocking_pipe_header *);
extern int blocking_getnameinfo(blocking_child *,
				blocking_pipe_header *);

#ifdef TEST_BLOCKING_WORKER
extern void gai_test_callback(int rescode, int gai_errno,
			      void *context, const char *name,
			      const char *service,
			      const struct addrinfo *hints,
			      const struct addrinfo *ai_res);
extern void gni_test_callback(int rescode, int gni_errno,
			      sockaddr_u *psau, int flags,
			      const char *host,
			      const char *service, void *context);
#endif	/* TEST_BLOCKING_WORKER */
#endif	/* WORKER */

#endif	/* INTRESWORK_H */
