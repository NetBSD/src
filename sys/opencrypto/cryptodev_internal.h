/* $NetBSD: cryptodev_internal.h,v 1.3.48.2 2026/06/27 15:23:23 martin Exp $ */

/* exported to compat code, not for consumers */

#ifndef	_SYS_OPENCRYPTO_CRYPTODEV_INTERNAL_H_
#define	_SYS_OPENCRYPTO_CRYPTODEV_INTERNAL_H_

#include <sys/types.h>

struct crypt_n_op;
struct crypt_op;
struct csession;
struct fctyp;
struct lwp;
struct session_n_op;
struct session_op;

#define CRYPTODEV_OPS_MAX 1000000

int cryptodev_op(struct csession *, struct crypt_op *, struct lwp *);
int cryptodev_mop(struct fcrypt *, struct crypt_n_op *, size_t, struct lwp *);
int cryptodev_session(struct fcrypt *, struct session_op *);
int cryptodev_msession(struct fcrypt *, struct session_n_op *, size_t);
void cryptodev_cse_free(struct csession *);
struct csession *cryptodev_cse_find(struct fcrypt *, uint32_t);

#endif	/* _SYS_OPENCRYPTO_CRYPTODEV_INTERNAL_H_ */
