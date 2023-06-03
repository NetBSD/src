#	$NetBSD: Makefile,v 1.21 2023/06/03 21:28:53 lukem Exp $

PROG=	newsyslog
SRCS=	newsyslog.c

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.own.mk>

COPTS.newsyslog.c +=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
