#       $NetBSD: Makefile,v 1.1.1.1.8.2 2000/11/20 20:31:22 bouyer Exp $

PROG=   chrpicontoppm
MKMAN=no

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-L${PBMROOT}/lib -lpnm -lppm -lpgm -lpbm

.include <bsd.prog.mk>

