#	$NetBSD: Makefile,v 1.27 2003/04/26 19:10:09 fvdl Exp $

SUBDIR=	mbr
.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+= mbr_bootsel
.endif

.if (${MACHINE} == "i386" || \
     ${MACHINE} == "bebox" || \
     ${MACHINE} == "cobalt" || \
     ${MACHINE} == "ofppc" || \
     ${MACHINE} == "hpcarm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "arc" || \
     ${MACHINE} == "prep" || \
     ${MACHINE} == "playstation2" || \
     ${MACHINE} == "mvmeppc" || \
     ${MACHINE} == "netwinder" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE} == "amd64")
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
