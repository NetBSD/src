#	$NetBSD: Makefile,v 1.2.8.2 2008/10/19 22:18:06 haad Exp $
#

PROG=	rumptest
NOMAN=	woman

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
