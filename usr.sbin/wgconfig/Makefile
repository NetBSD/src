#	$NetBSD: Makefile,v 1.1 2020/08/20 21:28:02 riastradh Exp $

PROG=	wgconfig
MAN=	wgconfig.8

DPADD+=	${LIBPROP} ${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lprop -lcrypt -lutil

.include <bsd.prog.mk>
