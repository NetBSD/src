#	$NetBSD: Makefile.inc,v 1.3 2019/11/04 12:45:10 joerg Exp $

.include <bsd.own.mk>

# Our sources
.PATH:	${.PARSEDIR} ${ARCHDIR}/tls

SRCS+=			tls.c
CPPFLAGS.tls.c+=	-D_LIBC_SOURCE -std=gnu11
