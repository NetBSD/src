#	$NetBSD: Makefile,v 1.10.2.1 1999/04/18 00:18:49 fvdl Exp $


.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.if ${MACHINE} == "i386"
SUBDIR=	mbr mbr_bootsel
.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
