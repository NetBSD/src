#	$NetBSD: Makefile,v 1.9 2005/04/23 16:54:09 christos Exp $

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c misc.c
MAN=	sftp.1

BINDIR=	/usr/bin

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
