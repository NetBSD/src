#	$NetBSD: Makefile,v 1.1 2001/01/14 05:22:33 itojun Exp $

.include <bsd.own.mk>

PROG=	sftp-server
MAN=	sftp-server.8

BINDIR=	/usr/libexec

SRCS=	sftp-server.c log-server.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
