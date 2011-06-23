#	$NetBSD: Makefile,v 1.3.4.1 2011/06/23 14:17:50 cherry Exp $

BINDIR=	/usr/bin

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c
MAN=	sftp.1

LDADD+=	-ledit -lterminfo
DPADD+=	${LIBEDIT} ${LIBTERMINFO}

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.sftp.c+=		-Wno-pointer-sign
COPTS.sftp-client.c+=	-Wno-pointer-sign
.endif
