#	$NetBSD: Makefile,v 1.21 2002/02/27 22:31:18 scw Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || ${MACHINE} == "bebox" || ${MACHINE} == "cobalt" ||\
    ${MACHINE} == "ofppc" || ${MACHINE} == "hpcmips" || ${MACHINE} == "arc" ||\
    ${MACHINE} == "prep" || ${MACHINE} == "playstation2" ||\
    ${MACHINE} == "mvmeppc")
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
