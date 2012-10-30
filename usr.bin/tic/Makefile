#	$NetBSD: Makefile,v 1.1.6.1 2012/10/30 19:00:27 yamt Exp $

PROG=		tic
WARNS=		4

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo

.ifndef HOSTPROG
LDADD+=		-lterminfo -lutil
DPADD+=		${LIBTERMINFO} ${LIBUTIL}
.endif

.include <bsd.prog.mk>
