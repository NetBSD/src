#	$NetBSD: Makefile,v 1.11 1999/04/15 22:31:23 fvdl Exp $


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
