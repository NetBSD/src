#	$NetBSD: Makefile,v 1.12.4.1 2002/03/06 23:35:18 he Exp $

PROG=	newsyslog
SRCS=	newsyslog.c
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.prog.mk>
