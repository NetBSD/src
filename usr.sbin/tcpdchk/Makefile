#	$NetBSD: Makefile,v 1.5 1998/01/09 08:11:47 perry Exp $
#

WARNS?=	0
PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${.CURDIR}/../../lib/libwrap
CPPFLAGS+=-DFACILITY=LOG_AUTHPRIV -DSEVERITY=LOG_INFO -DPARANOID
CPPFLAGS+=-DREAL_DAEMON_DIR=\"/usr/libexec\" -DHOSTS_ACCESS -DDAEMON_UMASK=022
CPPFLAGS+=-DRFC931_TIMEOUT=10 -DALWAYS_HOSTNAME -DSYS_ERRLIST_DEFINED
CPPFLAGS+=-DHOSTS_ALLOW=\"/etc/hosts.allow\" -DHOSTS_DENY=\"/etc/hosts.deny\"
CPPFLAGS+=-DPROCESS_OPTIONS

.include <bsd.prog.mk>
