#	$NetBSD: Makefile,v 1.18 2006/08/26 18:18:16 christos Exp $

.include <bsd.own.mk>

PROG=	makewhatis
SRCS=	makewhatis.c manconf.c
.PATH:	${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD+=	${LIBZ}
LDADD+=	-lz
.endif
DPADD+= ${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
