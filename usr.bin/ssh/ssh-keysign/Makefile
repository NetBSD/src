#	$NetBSD: Makefile,v 1.3.2.3 2002/07/03 23:01:48 lukem Exp $

.include <bsd.own.mk>

PROG=	ssh-keysign

SRCS=	ssh-keysign.c readconf.c

BINOWN=	root
BINMODE=4555

BINDIR=	/usr/libexec
MAN=	ssh-keysign.8

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lz
DPADD+=	${LIBCRYPTO} ${LIBZ}
