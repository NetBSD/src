/*	$NetBSD: pwd.h,v 1.1.2.1 2008/11/08 21:49:34 christos Exp $	*/

#ifndef _COMPAT_PWD_H_
#define	_COMPAT_PWD_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <sys/types.h>

struct passwd;
struct passwd50 {
	__aconst char *pw_name;		/* user name */
	__aconst char *pw_passwd;	/* encrypted password */
	uid_t	       pw_uid;		/* user uid */
	gid_t	       pw_gid;		/* user gid */
	int32_t	       pw_change;	/* password change time */
	__aconst char *pw_class;	/* user login class */
	__aconst char *pw_gecos;	/* general information */
	__aconst char *pw_dir;		/* home directory */
	__aconst char *pw_shell;	/* default shell */
	int32_t        pw_expire;	/* account expiration */
};

__BEGIN_DECLS
struct passwd50	*getpwuid(uid_t);
struct passwd50	*getpwnam(const char *);
#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
int		 getpwnam_r(const char *, struct passwd50 *, char *, size_t,
    struct passwd50 **);
int		 getpwuid_r(uid_t, struct passwd50 *, char *, size_t,
    struct passwd50 **);
#endif
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
struct passwd50	*getpwent(void);
#endif
#if defined(_NETBSD_SOURCE)
int		 pw_scan(char *, struct passwd50 *, int *);
int		 getpwent_r(struct passwd50 *, char *, size_t, struct passwd50 **);
#endif
#if defined(_NETBSD_SOURCE)
int		 pwcache_userdb(int (*)(int), void (*)(void),
    struct passwd50 * (*)(const char *), struct passwd50 * (*)(uid_t));
#endif

struct passwd	*__getpwuid50(uid_t);
struct passwd	*__getpwnam50(const char *);
#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_REENTRANT) || defined(_NETBSD_SOURCE)
int		 __getpwnam_r50(const char *, struct passwd *, char *, size_t,
    struct passwd **);
int		 __getpwuid_r50(uid_t, struct passwd *, char *, size_t,
    struct passwd **);
#endif
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
struct passwd	*__getpwent50(void);
#endif
#if defined(_NETBSD_SOURCE)
int		 __pw_scan50(char *, struct passwd *, int *);
int		 __getpwent_r50(struct passwd *, char *, size_t, struct passwd **);
#endif
int		 setpassent(int);
#if defined(_NETBSD_SOURCE)
int		 __pwcache_userdb50(int (*)(int), void (*)(void),
    struct passwd * (*)(const char *), struct passwd * (*)(uid_t));
#endif
__END_DECLS

#endif /* !_PWD_H_ */
