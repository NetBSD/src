#	$NetBSD: Makefile,v 1.11.2.1 2002/11/03 13:55:31 he Exp $

PROG=	makewhatis
SRCS=	makewhatis.c manconf.c
.PATH:	${.CURDIR}/../../usr.bin/man
CPPFLAGS+=-I${.CURDIR}/../../usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
.endif
