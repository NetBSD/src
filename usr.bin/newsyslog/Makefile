#	$NetBSD: Makefile,v 1.15 2003/10/26 01:56:54 lukem Exp $

PROG=	newsyslog
SRCS=	newsyslog.c

MAN=	newsyslog.8
MLINKS+=newsyslog.8 newsyslog.conf.5

.if ${MACHINE_ARCH} == "arm"
# XXX	gcc 3.3 generates a non-functional newsyslog without this
COPTS.newsyslog.c+=	-fno-gcse-lm
.endif

.include <bsd.prog.mk>
