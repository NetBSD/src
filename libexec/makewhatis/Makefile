#	$NetBSD: Makefile,v 1.19.26.1 2009/05/13 19:18:42 jym Exp $

WARNS?=	2	# XXX: const issues

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
