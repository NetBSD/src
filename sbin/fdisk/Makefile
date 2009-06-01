#	$NetBSD: Makefile,v 1.40 2009/06/01 19:57:33 christos Exp $

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
DPADD+=	${LIBUTIL} ${LIBZ}
LDADD+=	-lutil -lz
.include <bsd.subdir.mk>
.endif
