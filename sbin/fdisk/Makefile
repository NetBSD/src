#	$NetBSD: Makefile,v 1.16 1999/09/25 04:27:55 enami Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || \
    ${MACHINE} == "ofppc" || ${MACHINE} == "hpcmips")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

MAN=	fdisk.8

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
