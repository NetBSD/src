#	$NetBSD: Makefile,v 1.15 2002/09/13 15:56:37 thorpej Exp $

.ifndef HOSTPROG
.include <bsd.obj.mk>
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
