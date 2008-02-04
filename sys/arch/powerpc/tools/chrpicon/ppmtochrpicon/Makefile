#       $NetBSD: Makefile,v 1.2.32.1 2008/02/04 09:22:24 yamt Exp $

PROG=   ppmtochrpicon
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-Wl,-R${PBMROOT}/lib -L${PBMROOT}/lib -lnetpbm

.include <bsd.prog.mk>

