#       Makefile,v 1.2 2001/12/12 01:49:50 tv Exp

PROG=   ppmtochrpicon
NOMAN=# defined

PBMROOT=/usr/pkg

CPPFLAGS+= -I${.CURDIR}/.. -I${PBMROOT}/include
LDADD+=-Wl,-R${PBMROOT}/lib -L${PBMROOT}/lib -lnetpbm

.include <bsd.prog.mk>

