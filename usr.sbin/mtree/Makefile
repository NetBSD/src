#	$NetBSD: Makefile,v 1.23 2002/01/29 00:07:27 tv Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c excludes.c misc.c mtree.c spec.c verify.c \
	getid.c stat_flags.c pack_dev.c
WARNS?=	2

.ifndef HOSTPROG
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
.endif

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls -I${.CURDIR}/../../sbin/mknod
.PATH:	${.CURDIR}/../../bin/ls ${.CURDIR}/../../sbin/mknod

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
