#	$NetBSD: Makefile,v 1.4 2002/07/01 06:19:22 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh-keysign

SRCS=	ssh-keysign.c

BINOWN=	root
#BINMODE=4555

BINDIR=	/usr/libexec
MAN=	ssh-keysign.8

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
