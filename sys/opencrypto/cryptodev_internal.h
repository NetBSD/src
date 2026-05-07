/* $NetBSD: cryptodev_internal.h,v 1.3.48.1 2026/05/07 14:40:13 martin Exp $ */

/* exported to compat code, not for consumers */

extern kmutex_t cryptodev_mtx;

struct csession;
int cryptodev_op(struct csession *, struct crypt_op *, struct lwp *);
int cryptodev_mop(struct fcrypt *, struct crypt_n_op *, size_t, struct lwp *);
int cryptodev_session(struct fcrypt *, struct session_op *);
int cryptodev_msession(struct fcrypt *, struct session_n_op *, size_t);
void cryptodev_cse_free(struct csession *);
struct csession *cryptodev_cse_find(struct fcrypt *, uint32_t);
