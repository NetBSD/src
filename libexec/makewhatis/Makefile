#	$NetBSD: Makefile,v 1.17 2003/05/18 07:57:32 lukem Exp $

.include <bsd.own.mk>

PROG=	makewhatis
SRCS=	makewhatis.c manconf.c
.PATH:	${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz
.endif

.include <bsd.prog.mk>
