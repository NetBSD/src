#	$NetBSD: Makefile,v 1.17 2000/01/31 15:54:48 soda Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || \
    ${MACHINE} == "ofppc" || ${MACHINE} == "hpcmips" || ${MACHINE} == "arc")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

MAN=	fdisk.8

.if ${MACHINE} == "arc"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rsd0d"'
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
