/*      $NetBSD: compat_sigaltstack.h,v 1.1 2007/06/03 14:59:46 dsl Exp $        */

/* Wrapper for calling sigaltstack1() from compat (or other) code */

/* Maybe these definitions could be global. */
#ifdef SCARG_P32
/* compat32 */
#define	SCARG_COMPAT_PTR(uap,p)	SCARG_P32(uap, p)
#define	COMPAT_GET_PTR(p)	NETBSD32PTR64(p)
#define	COMPAT_SET_PTR(p, v)	NETBSD32PTR32(p, v)
#else
/* not a size change */
#define	SCARG_COMPAT_PTR(uap,p)	SCARG(uap, p)
#define	COMPAT_GET_PTR(p)	(p)
#define	COMPAT_SET_PTR(p, v)	((p) = (v))
#endif

#define compat_sigaltstack(uap, compat_ss) do { \
	struct compat_ss css; \
	struct sigaltstack nss, oss; \
	int error; \
\
	if (SCARG_COMPAT_PTR(uap, nss)) { \
		error = copyin(SCARG_COMPAT_PTR(uap, nss), &css, sizeof css); \
		if (error) \
			return error; \
		 nss.ss_sp = COMPAT_GET_PTR(css.ss_sp); \
		 nss.ss_size = css.ss_size; \
		 nss.ss_flags = css.ss_flags; \
	} \
\
	error = sigaltstack1(l, SCARG_COMPAT_PTR(uap, nss) ? &nss : 0, \
				SCARG_COMPAT_PTR(uap, oss) ? &oss : 0); \
	if (error) \
		return (error); \
\
	if (SCARG_COMPAT_PTR(uap, oss)) { \
		COMPAT_SET_PTR(css.ss_sp, oss.ss_sp); \
		css.ss_size = oss.ss_size; \
		css.ss_flags = oss.ss_flags; \
		error = copyout(&css, SCARG_COMPAT_PTR(uap, oss), sizeof(css)); \
		if (error) \
			return (error); \
	} \
	return (0); \
} while (0)
