#	$NetBSD: Makefile,v 1.6 2002/09/18 13:50:52 lukem Exp $

.include <bsd.own.mk>

PROG=	sftp

BINDIR=	/usr/bin
MAN=	sftp.1

SRCS=	sftp.c sftp-client.c sftp-int.c sftp-common.c sftp-glob.c misc.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

