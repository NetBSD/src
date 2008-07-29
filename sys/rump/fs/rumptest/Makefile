#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:47 pooka Exp $
#

PROG=	rumptest

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
