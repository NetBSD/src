#	$NetBSD: Makefile,v 1.2 2020/08/26 15:49:07 riastradh Exp $

PROG=	wgconfig
MAN=	wgconfig.8

DPADD+=	${LIBPROP} ${LIBUTIL}
LDADD+=	-lprop -lutil

.include <bsd.prog.mk>
