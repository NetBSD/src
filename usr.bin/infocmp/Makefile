#	$NetBSD: Makefile,v 1.1.12.1 2014/08/20 00:04:59 tls Exp $

PROG=		infocmp

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo
LDADD+=		-lterminfo
DPADD+=		${LIBTERMINFO}

.include <bsd.prog.mk>
