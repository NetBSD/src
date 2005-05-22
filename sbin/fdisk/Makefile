#	$NetBSD: Makefile,v 1.32 2005/05/22 03:45:30 lukem Exp $

SUBDIR=	mbr

PROG=	fdisk 
SRCS=	fdisk.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

MAN=	fdisk.8

.if ${MACHINE} == "arc"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rsd0d"'
.endif

.if ${MACHINE} == "netwinder"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rwd0c"'
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
