#	$NetBSD: Makefile,v 1.13.2.1 1999/12/27 18:30:23 wrstuden Exp $

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
