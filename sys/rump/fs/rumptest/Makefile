#	$NetBSD: Makefile,v 1.2.2.2 2008/07/31 04:51:05 simonb Exp $
#

PROG=	rumptest
NOMAN=	woman

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
