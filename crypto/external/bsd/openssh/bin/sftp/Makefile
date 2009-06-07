#	$NetBSD: Makefile,v 1.1 2009/06/07 22:38:45 christos Exp $

BINDIR=	/usr/bin

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c misc.c fmt_scaled.c
MAN=	sftp.1

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.sftp-client.c+=       -Wno-pointer-sign
.endif
