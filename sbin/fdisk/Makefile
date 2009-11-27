#	$NetBSD: Makefile,v 1.41 2009/11/27 15:37:32 tsutsui Exp $

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
DPADD+=	${LIBUTIL} ${LIBZ}
LDADD+=	-lutil -lz
.endif
