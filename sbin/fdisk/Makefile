#	$NetBSD: Makefile,v 1.9 1997/09/25 05:08:28 lukem Exp $


.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
