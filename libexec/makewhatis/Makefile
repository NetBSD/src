#	$NetBSD: Makefile,v 1.14 2002/09/04 16:57:47 junyoung Exp $

.ifndef HOSTPROG
.include <bsd.obj.mk>
.endif

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
