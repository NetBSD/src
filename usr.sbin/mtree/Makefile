#	$NetBSD: Makefile,v 1.9 1997/05/06 20:46:26 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	mtree
#CFLAGS+=-DDEBUG
MAN=	mtree.8
SRCS=	compare.c crc.c create.c misc.c mtree.c spec.c verify.c
.PATH.c: ${.CURDIR}/../../usr.bin/cksum

.include <bsd.prog.mk>
