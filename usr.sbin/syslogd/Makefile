#	$NetBSD: Makefile,v 1.23.6.1 2012/10/30 19:00:52 yamt Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
.include <bsd.own.mk>

WARNS?=4
USE_FORT?= yes	# network server

LINTFLAGS+=-X 132,247,135,259,117,298

PROG=	syslogd
SRCS=	syslogd.c utmpentry.c tls.c sign.c
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL} ${LIBEVENT}
LDADD+=-lutil -levent
#make symlink to old socket location for transitional period
SYMLINKS=	/var/run/log /dev/log
.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP -Wredundant-decls

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.if ${MKCRYPTO} != "no"
LDADD+=	-lssl -lcrypto
.else
CPPFLAGS+=-DDISABLE_TLS -DDISABLE_SIGN
.endif

.include <bsd.prog.mk>
