#	$NetBSD: Makefile,v 1.2.6.2 2008/09/28 10:41:03 mjf Exp $
#

PROG=	rumptest
NOMAN=	woman

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
