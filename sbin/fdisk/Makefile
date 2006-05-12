#	$NetBSD: Makefile,v 1.36 2006/05/12 01:18:25 mrg Exp $

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

.if !defined(HOSTPROGNAME)
.if ${HAVE_GCC} == 4
COPTS.fdisk.c+=	-Wno-pointer-sign
.endif
.endif
