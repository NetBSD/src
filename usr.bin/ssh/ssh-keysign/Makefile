#	$NetBSD: Makefile,v 1.3.2.2 2002/06/26 16:54:51 tv Exp $

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
