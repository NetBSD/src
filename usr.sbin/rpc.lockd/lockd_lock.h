/*	$NetBSD: lockd_lock.h,v 1.2.28.1 2007/11/06 23:36:35 matt Exp $	*/

/* Headers and function declarations for file-locking utilities */

struct nlm4_holder *testlock(struct nlm4_lock *, int);

enum nlm_stats getlock(nlm4_lockargs *, struct svc_req *, int);
enum nlm_stats unlock(nlm4_lock *, int);
void notify(const char *, int);

/* flags for testlock, getlock & unlock */
#define LOCK_ASYNC	0x01 /* async version (getlock only) */
#define LOCK_V4 	0x02 /* v4 version */
#define LOCK_MON 	0x04 /* monitored lock (getlock only) */
#define LOCK_CANCEL 0x08 /* cancel, not unlock request (unlock only) */

/* callbacks from lock_proc.c */
void	transmit_result(int, nlm_res *, struct sockaddr *);
void	transmit4_result(int, nlm4_res *, struct sockaddr *);
CLIENT  *get_client(struct sockaddr *, rpcvers_t);
