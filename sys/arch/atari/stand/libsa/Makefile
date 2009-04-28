#	$NetBSD: Makefile,v 1.27.86.2 2009/04/28 07:33:54 skrll Exp $

# Logically src/sys
S!=	cd ${.CURDIR}/../../../..; pwd

.include "../Makefile.booters"

CPPFLAGS+=	-I${S} -I.

.include <bsd.prog.mk>

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_EXTRADIR=${.CURDIR}
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

LIBS=	${LIBSA} ${LIBKERN}

realall: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
