#	$NetBSD: Makefile,v 1.2 2008/07/29 14:00:01 pooka Exp $
#

PROG=	rumptest
NOMAN=	woman

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
