#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 1997/06/29 18:57:44 christos Exp $

CFLAGS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
PROG=	syslogd
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
