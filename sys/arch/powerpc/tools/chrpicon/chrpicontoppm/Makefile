#       $NetBSD: Makefile,v 1.1.1.1.16.1 2002/01/08 00:27:14 nathanw Exp $

PROG=   chrpicontoppm
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

