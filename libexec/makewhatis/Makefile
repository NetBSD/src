#	$NetBSD: Makefile,v 1.16 2002/09/18 06:24:35 lukem Exp $

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

PROG=	makewhatis
SRCS=	makewhatis.c manconf.c
.PATH:	${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
.endif
