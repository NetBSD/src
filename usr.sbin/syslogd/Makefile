#	$NetBSD: Makefile,v 1.14 1999/12/02 16:17:29 itojun Exp $
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

.include <bsd.prog.mk>
