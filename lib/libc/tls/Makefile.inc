#	$NetBSD: Makefile.inc,v 1.1 2011/03/09 23:10:06 joerg Exp $

.include <bsd.own.mk>

# Our sources
.PATH:	${.PARSEDIR} ${ARCHDIR}/tls

SRCS+=	tls.c
