/* netbsd */
#define	SYSCONFDIR		"/etc"
#define	SBINDIR			"/sbin"
#define	LIBDIR			"/lib"
#define	LIBEXECDIR		"/libexec"
#define	DBDIR			"/var/db/dhcpcd"
#define	RUNDIR			"/var/run"
#include			"compat/pidfile.h"
#define	HAVE_SETPROCTITLE
#define	HAVE_SYS_QUEUE_H
#include			"compat/reallocarray.h"
#define	HAVE_REALLOCARRAY
#define	HAVE_KQUEUE
#define	HAVE_KQUEUE1
#define	HAVE_SYS_BITOPS_H
#define	HAVE_MD5_H
#define	SHA2_H			<sha2.h>
#include			"compat/crypt/hmac.h"
