#	from: @(#)Makefile	5.7 (Berkeley) 9/30/90
#	$Id: Makefile,v 1.3 1993/07/30 20:50:51 mycroft Exp $

PROG=	syslogd
SRCS=	syslogd.c ttymsg.c
.PATH:	${.CURDIR}/../../usr.bin/wall
MAN5=	syslog.conf.0
MAN8=	syslogd.0
DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>
