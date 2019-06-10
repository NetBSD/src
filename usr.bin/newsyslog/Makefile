#	$NetBSD: Makefile,v 1.18.46.1 2019/06/10 22:10:22 christos Exp $

PROG=	newsyslog
SRCS=	newsyslog.c

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.newsyslog.c += -Wno-error=format-truncation
.endif

.include <bsd.prog.mk>
