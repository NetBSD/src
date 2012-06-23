#	$NetBSD: Makefile,v 1.1.8.1 2012/06/23 22:54:56 riz Exp $

PROG=		tic
WARNS=		4

CPPFLAGS+=	-I${.CURDIR}/../../lib/libterminfo

.ifndef HOSTPROG
LDADD+=		-lterminfo -lutil
DPADD+=		${LIBTERMINFO} ${LIBUTIL}
.endif

.include <bsd.prog.mk>
