#	$NetBSD: Makefile,v 1.17 2002/08/02 02:23:49 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	syslogd
SRCS=	syslogd.c utmpentry.c
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL}
LDADD+=-lutil
#make symlink to old socket location for transitional period
SYMLINKS=	/var/run/log /dev/log
CPPFLAGS+=-DINET6
.PATH.c: ${.CURDIR}/../../usr.bin/who
CPPFLAGS+=-I${.CURDIR}/../../usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>
