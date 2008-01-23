#       $NetBSD: Makefile,v 1.3 2008/01/23 23:22:02 garbled Exp $

PROG=   ppmtochrpicon
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-Wl,-R${PBMROOT}/lib -L${PBMROOT}/lib -lnetpbm

.include <bsd.prog.mk>

