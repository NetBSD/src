/*	$NetBSD: lockd_lock.h,v 1.1 2000/06/07 14:34:40 bouyer Exp $	*/

/* Headers and function declarations for file-locking utilities */

struct nlm4_holder * testlock __P((struct nlm4_lock *, int));

enum nlm_stats getlock __P((nlm4_lockargs *, struct svc_req *, int));
enum nlm_stats unlock __P((nlm4_lock *, int));
void notify __P((char *, int));

/* flags for testlock, getlock & unlock */
#define LOCK_ASYNC	0x01 /* async version (getlock only) */
#define LOCK_V4 	0x02 /* v4 version */
#define LOCK_MON 	0x04 /* monitored lock (getlock only) */
#define LOCK_CANCEL 0x08 /* cancel, not unlock request (unlock only) */

/* callbacks from lock_proc.c */
void	transmit_result __P((int, nlm_res *, struct sockaddr *));
void	transmit4_result __P((int, nlm4_res *, struct sockaddr *));
CLIENT  *get_client __P((struct sockaddr_in *, u_long));
