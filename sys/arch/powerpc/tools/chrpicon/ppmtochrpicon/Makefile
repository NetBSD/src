#       $NetBSD: Makefile,v 1.2.100.1 2008/02/18 21:04:59 mjf Exp $

PROG=   ppmtochrpicon
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-Wl,-R${PBMROOT}/lib -L${PBMROOT}/lib -lnetpbm

.include <bsd.prog.mk>

