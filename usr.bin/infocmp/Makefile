#	$NetBSD: Makefile,v 1.3 2016/03/09 20:02:33 christos Exp $

PROG=		infocmp

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo
LDADD+=		-lterminfo -lutil
DPADD+=		${LIBTERMINFO} ${LIBUTIL}

.include <bsd.prog.mk>
