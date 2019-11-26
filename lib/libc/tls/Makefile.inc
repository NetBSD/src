#	$NetBSD: Makefile.inc,v 1.2.46.1 2019/11/26 08:12:26 martin Exp $

.include <bsd.own.mk>

# Our sources
.PATH:	${.PARSEDIR} ${ARCHDIR}/tls

SRCS+=			tls.c
CPPFLAGS.tls.c+=	-D_LIBC_SOURCE -std=gnu11
