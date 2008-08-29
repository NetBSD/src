#	$NetBSD: Makefile,v 1.11 2008/08/29 00:02:24 gmcgarry Exp $

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c misc.c
MAN=	sftp.1

BINDIR=	/usr/bin

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.sftp-client.c+=       -Wno-pointer-sign
.endif
