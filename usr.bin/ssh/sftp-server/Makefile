#	$NetBSD: Makefile,v 1.2 2001/02/07 17:05:37 itojun Exp $

.include <bsd.own.mk>

PROG=	sftp-server
MAN=	sftp-server.8

BINDIR=	/usr/libexec

SRCS=	sftp-server.c sftp-common.c log-server.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
