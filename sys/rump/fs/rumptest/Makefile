#	$NetBSD: Makefile,v 1.2.4.2 2008/09/18 04:37:04 wrstuden Exp $
#

PROG=	rumptest
NOMAN=	woman

LDADD=	${RUMPFSLDADD} -lrump -lrumpuser -lpthread
LDFLAGS+=-Wl,--wrap=malloc

.include "${.CURDIR}/../Makefile.rumpfs"
.include <bsd.prog.mk>
