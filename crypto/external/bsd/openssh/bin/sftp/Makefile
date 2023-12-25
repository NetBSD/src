#	$NetBSD: Makefile,v 1.10.2.3 2023/12/25 12:22:53 martin Exp $

BINDIR=	/usr/bin

PROG=	sftp
SRCS=	sftp.c sftp-client.c sftp-common.c sftp-glob.c sftp-usergroup.c
MAN=	sftp.1

LDADD+=	-ledit -lterminfo
DPADD+=	${LIBEDIT} ${LIBTERMINFO}

COPTS.sftp.c+=		-Wno-pointer-sign
COPTS.sftp-client.c+=	-Wno-pointer-sign

.include <bsd.own.mk>

COPTS.sftp.c+=		${GCC_NO_IMPLICIT_FALLTHROUGH}

.include <bsd.prog.mk>
