#	$NetBSD: Makefile,v 1.11 2002/03/08 21:59:07 jdolecek Exp $

PROG=	makewhatis
SRCS=	makewhatis.c config.c
.PATH:	${.CURDIR}/../../usr.bin/man
CPPFLAGS+=-I${.CURDIR}/../../usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
.endif
