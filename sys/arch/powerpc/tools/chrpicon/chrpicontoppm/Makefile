#       $NetBSD: Makefile,v 1.1.1.1.12.1 2002/01/10 19:48:08 thorpej Exp $

PROG=   chrpicontoppm
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

