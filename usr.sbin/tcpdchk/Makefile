#	$Netbsd$

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CFLAGS+= -I${.CURDIR}/../../lib/libwrap

CFLAGS+=-DFACILITY=LOG_AUTHPRIV -DSEVERITY=LOG_INFO -DPARANOID
CFLAGS+=-DREAL_DAEMON_DIR=\"/usr/libexec\" -DHOSTS_ACCESS -DDAEMON_UMASK=022
CFLAGS+=-DRFC931_TIMEOUT=10 -DALWAYS_HOSTNAME -DSYS_ERRLIST_DEFINED
CFLAGS+=-DHOSTS_ALLOW=\"/etc/hosts.allow\" -DHOSTS_DENY=\"/etc/hosts.deny\"
CFLAGS+=-DPROCESS_OPTIONS

.include <bsd.prog.mk>
