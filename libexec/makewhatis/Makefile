#	$NetBSD: Makefile,v 1.12 2002/08/19 13:54:36 lukem Exp $

.include <bsd.obj.mk>

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
