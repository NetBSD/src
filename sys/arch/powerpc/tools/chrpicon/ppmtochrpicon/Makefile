#       $NetBSD: Makefile,v 1.1.1.1 1999/11/19 00:43:20 lonhyn Exp $

PROG=   ppmtochrpicon
MKMAN=no

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

