#	$NetBSD: Makefile,v 1.16 2001/09/10 03:22:24 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c \
	stat_flags.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls
.PATH:	${.CURDIR}/../../usr.bin/cksum ${.CURDIR}/../../bin/ls

.include <bsd.prog.mk>
