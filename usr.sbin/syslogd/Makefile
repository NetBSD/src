#	$NetBSD: Makefile,v 1.18 2002/09/18 03:54:37 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	syslogd
SRCS=	syslogd.c utmpentry.c
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL}
LDADD+=-lutil
#make symlink to old socket location for transitional period
SYMLINKS=	/var/run/log /dev/log
CPPFLAGS+=-DINET6
.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>
