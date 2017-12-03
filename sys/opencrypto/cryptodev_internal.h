/* $NetBSD: cryptodev_internal.h,v 1.1.20.1 2017/12/03 11:39:06 jdolecek Exp $ */

/* exported to compat code, not for consumers */

extern kmutex_t cryptodev_mtx;

struct csession;
int cryptodev_op(struct csession *, struct crypt_op *, struct lwp *);
int cryptodev_mop(struct fcrypt *, struct crypt_n_op *, int, struct lwp *);
int cryptodev_session(struct fcrypt *, struct session_op *);
int cryptodev_msession(struct fcrypt *, struct session_n_op *, int);
struct csession *cryptodev_csefind(struct fcrypt *fcr, u_int ses);
