#	$NetBSD: Makefile,v 1.37.4.1 2008/01/09 01:38:03 matt Exp $

WARS=	4
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
