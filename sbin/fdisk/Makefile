#	$NetBSD: Makefile,v 1.37 2007/03/10 18:22:04 apb Exp $

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

.if !defined(HOSTPROGNAME)
.if ${HAVE_GCC} == 4
COPTS.fdisk.c+=	-Wno-pointer-sign
.endif
.endif
