#	$NetBSD: Makefile,v 1.1.2.1 2005/04/13 16:05:49 tron Exp $
#	$OpenBSD: Makefile,v 1.3 2004/01/21 08:07:41 deraadt Exp $

PROG=	pfspamd-setup
SRCS=	spamd-setup.c
MAN=	pfspamd-setup.8

LDADD=	-lz
DPADD=	${LIBZ}

BINDIR=	/usr/libexec

pfspamd-setup.8: spamd-setup.8
	cp $> $@

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd-setup
