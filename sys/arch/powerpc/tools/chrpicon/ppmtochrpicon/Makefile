#       $NetBSD: Makefile,v 1.1.1.1.2.1 1999/12/27 18:33:42 wrstuden Exp $

PROG=   ppmtochrpicon
MKMAN=no

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

