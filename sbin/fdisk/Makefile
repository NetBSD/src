#	$NetBSD: Makefile,v 1.31 2005/01/20 16:39:23 xtraeme Exp $

SUBDIR=	mbr

.if (${MACHINE} == "i386" || \
     ${MACHINE} == "bebox" || \
     ${MACHINE} == "cobalt" || \
     ${MACHINE} == "ofppc" || \
     ${MACHINE} == "hpcarm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "iyonix" || \
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
