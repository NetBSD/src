#       $NetBSD: Makefile,v 1.2 2001/12/12 01:49:49 tv Exp $

PROG=   chrpicontoppm
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

