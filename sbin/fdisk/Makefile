#	$NetBSD: Makefile,v 1.20 2001/10/15 16:22:53 uch Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || ${MACHINE} == "cobalt" ||\
    ${MACHINE} == "ofppc" || ${MACHINE} == "hpcmips" || ${MACHINE} == "arc" ||\
    ${MACHINE} == "prep" || ${MACHINE} == "playstation2")
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
