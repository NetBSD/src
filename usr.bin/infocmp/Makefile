#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 11:42:45 yamt Exp $

PROG=		infocmp

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo
LDADD+=		-lterminfo
DPADD+=		${LIBTERMINFO}

.include <bsd.prog.mk>
