#	$NetBSD: Makefile,v 1.13 2000/07/07 10:52:41 ad Exp $

PROG=	newsyslog
SRCS=	newsyslog.c
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.prog.mk>
