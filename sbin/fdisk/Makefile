#	$NetBSD: Makefile,v 1.38.16.1 2009/05/13 19:19:00 jym Exp $

PROG=	fdisk 
SRCS=	fdisk.c

MAN=	fdisk.8

.if ${MACHINE} == "arc"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rsd0d"'
.endif

.if ${MACHINE} == "netwinder"
CPPFLAGS+= -D_PATH_DEFDISK='"/dev/rwd0c"'
.endif

.include <bsd.prog.mk>

.if (${HOSTPROG:U} == "")
SUBDIR=	mbr
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.include <bsd.subdir.mk>
.endif
