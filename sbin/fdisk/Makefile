#	$NetBSD: Makefile,v 1.33 2005/06/12 19:14:10 dyoung Exp $

.if (${HOSTPROG:U} == "")
SUBDIR=	mbr
.endif

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
.if (${HOSTPROG:U} == "")
.include <bsd.subdir.mk>
.endif
