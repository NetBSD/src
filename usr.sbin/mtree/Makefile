#	$NetBSD: Makefile,v 1.19 2001/10/18 05:06:01 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c \
	stat_flags.c pack_dev.c
WARNS?=	2

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls -I${.CURDIR}/../../sbin/mknod
.PATH:	${.CURDIR}/../../usr.bin/cksum \
	${.CURDIR}/../../bin/ls \
	${.CURDIR}/../../sbin/mknod

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
