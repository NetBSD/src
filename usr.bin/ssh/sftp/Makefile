#	$NetBSD: Makefile,v 1.10 2006/05/11 23:16:30 mrg Exp $

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c misc.c
MAN=	sftp.1

BINDIR=	/usr/bin

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.sftp-client.c+=       -Wno-pointer-sign
.endif
