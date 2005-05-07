#	$NetBSD: Makefile,v 1.1.2.4 2005/05/07 14:55:17 riz Exp $
#	$OpenBSD: Makefile,v 1.3 2004/01/21 08:07:41 deraadt Exp $

PROG=	pfspamd-setup
SRCS=	spamd-setup.c
MAN=	pfspamd-setup.8

LDADD=	-lz
DPADD=	${LIBZ}

CPPFLAGS+=	-DPATH_SPAMD_CONF=\"/etc/pfspamd.conf\"

BINDIR=	/usr/libexec

pfspamd-setup.8: spamd-setup.8
	cp $> $@

CLEANFILES+=	pfspamd-setup.8

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/libexec/spamd-setup
