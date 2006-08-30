#	$NetBSD: Makefile,v 1.19 2006/08/30 12:11:40 christos Exp $

.include <bsd.own.mk>

PROG=	makewhatis
SRCS=	makewhatis.c manconf.c
.PATH:	${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin

MAN=	${PROG}.8

.ifndef HOSTPROG
DPADD+=	${LIBUTIL} ${LIBZ}
LDADD+=	-lutil -lz
.endif

.include <bsd.prog.mk>
