#	$NetBSD: Makefile,v 1.10 1998/10/15 15:23:23 ws Exp $


.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.if ${MACHINE} == "i386"
SUBDIR=	mbr
.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
