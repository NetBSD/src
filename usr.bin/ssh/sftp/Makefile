#	$OpenBSD: Makefile,v 1.1 2001/02/04 11:11:56 djm Exp $

.include <bsd.own.mk>

PROG=	sftp
BINOWN=	root

BINDIR=	/usr/bin
MAN=	sftp.1

SRCS=	sftp.c sftp-client.c sftp-int.c sftp-common.c sftp-glob.c misc.c

.include <bsd.prog.mk>

LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}

