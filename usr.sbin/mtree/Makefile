#	$NetBSD: Makefile,v 1.20 2001/11/07 08:01:51 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c excludes.c misc.c mtree.c spec.c verify.c \
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
