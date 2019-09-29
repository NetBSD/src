#	$NetBSD: Makefile,v 1.7 2019/09/29 23:44:58 mrg Exp $

BINDIR=	/usr/bin

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c
MAN=	sftp.1

LDADD+=	-ledit -lterminfo
DPADD+=	${LIBEDIT} ${LIBTERMINFO}

COPTS.sftp.c+=		-Wno-pointer-sign
COPTS.sftp-client.c+=	-Wno-pointer-sign

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.sftp.c+=		-Wno-error=implicit-fallthrough
.endif

.include <bsd.prog.mk>
