#	$NetBSD: Makefile,v 1.2 2002/06/24 05:52:29 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keysign

SRCS=	ssh-keysign.c

BINOWN=	root
BINMODE=4555

BINDIR=	/usr/libexec

MAN=	ssh-keysign.8

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
