#	$NetBSD: Makefile,v 1.1 2004/06/22 15:53:18 itojun Exp $
#	$OpenBSD: Makefile,v 1.3 2004/01/21 08:07:41 deraadt Exp $

PROG=	spamd-setup
SRCS=	spamd-setup.c
MAN=	spamd-setup.8

LDADD=	-lz
DPADD=	${LIBZ}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd-setup
