#	$NetBSD: Makefile,v 1.17 2001/10/09 04:50:00 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c \
	stat_flags.c pack_dev.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls -I${.CURDIR}/../../sbin/mknod
.PATH:	${.CURDIR}/../../usr.bin/cksum \
	${.CURDIR}/../../bin/ls \
	${.CURDIR}/../../sbin/mknod

.include <bsd.prog.mk>
