#	$NetBSD: Makefile,v 1.10.2.2 1999/05/02 21:25:05 perry Exp $

SUBDIR=	mbr

.if (${MACHINE} == "i386" || ${MACHINE} == "powerpc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
SUBDIR+= mbr_bootsel
.endif


MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
