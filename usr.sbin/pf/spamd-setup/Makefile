#	$NetBSD: Makefile,v 1.1 2004/11/11 11:54:51 yamt Exp $
#	$OpenBSD: Makefile,v 1.3 2004/01/21 08:07:41 deraadt Exp $

PROG=	spamd-setup
SRCS=	spamd-setup.c
MAN=	spamd-setup.8

LDADD=	-lz
DPADD=	${LIBZ}

BINDIR=	/usr/libexec

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd-setup
