#	$NetBSD: Makefile,v 1.11.6.1 2009/05/13 19:20:06 jym Exp $

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c misc.c fmt_scaled.c
MAN=	sftp.1

BINDIR=	/usr/bin

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.sftp-client.c+=       -Wno-pointer-sign
.endif
