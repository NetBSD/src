#	$NetBSD: Makefile,v 1.13.2.1 1999/11/20 16:46:38 he Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/27/95

PROG=	mtree
#CPPFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c \
	stat_flags.c code.c

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls
.PATH:	${.CURDIR}/../../usr.bin/cksum ${.CURDIR}/../../bin/ls

.include <bsd.prog.mk>
