#	$NetBSD: Makefile.inc,v 1.2.44.1 2020/04/13 08:03:11 martin Exp $

.include <bsd.own.mk>

# Our sources
.PATH:	${.PARSEDIR} ${ARCHDIR}/tls

SRCS+=			tls.c
CPPFLAGS.tls.c+=	-D_LIBC_SOURCE -std=gnu11
