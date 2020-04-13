#	$NetBSD: Makefile,v 1.18.46.2 2020/04/13 08:05:45 martin Exp $

PROG=	newsyslog
SRCS=	newsyslog.c

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.newsyslog.c += -Wno-error=format-truncation
.endif

.include <bsd.prog.mk>
