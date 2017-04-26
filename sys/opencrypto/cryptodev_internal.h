/* $NetBSD: cryptodev_internal.h,v 1.1.42.1 2017/04/26 02:53:30 pgoyette Exp $ */

/* exported to compat code, not for consumers */

extern kmutex_t crypto_mtx;

struct csession;
int cryptodev_op(struct csession *, struct crypt_op *, struct lwp *);
int cryptodev_mop(struct fcrypt *, struct crypt_n_op *, int, struct lwp *);
int cryptodev_session(struct fcrypt *, struct session_op *);
int cryptodev_msession(struct fcrypt *, struct session_n_op *, int);
struct csession *cryptodev_csefind(struct fcrypt *fcr, u_int ses);
