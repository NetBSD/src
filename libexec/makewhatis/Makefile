#	$NetBSD: Makefile,v 1.13 2002/09/04 06:39:58 junyoung Exp $

PROG=	makewhatis
SRCS=	makewhatis.c config.c
.PATH:	${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
.endif
