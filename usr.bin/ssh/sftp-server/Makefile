#	$NetBSD: Makefile,v 1.3 2001/03/19 20:03:25 itojun Exp $

.include <bsd.own.mk>

PROG=	sftp-server
MAN=	sftp-server.8

BINDIR=	/usr/libexec

SRCS=	sftp-server.c sftp-common.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
