#	$NetBSD: Makefile,v 1.22 2002/04/03 03:17:36 thorpej Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || \
     ${MACHINE} == "bebox" || \
     ${MACHINE} == "cobalt" || \
     ${MACHINE} == "ofppc" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "arc" || \
     ${MACHINE} == "prep" || \
     ${MACHINE} == "playstation2" || \
     ${MACHINE} == "mvmeppc" || \
     ${MACHINE} == "netwinder")
PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

MAN=	fdisk.8

.if ${MACHINE} == "arc"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rsd0d"'
.endif

.if ${MACHINE} == "netwinder"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rwd0c"'
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
