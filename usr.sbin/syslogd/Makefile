#	$NetBSD: Makefile,v 1.15 2001/07/01 16:23:42 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	syslogd
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL}
LDADD+=-lutil
#make symlink to old socket location for transitional period
SYMLINKS=	/var/run/log /dev/log
CPPFLAGS+=-DINET6

# KAME scopeid hack
#CPPFLAGS+=-DKAME_SCOPEID

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>
