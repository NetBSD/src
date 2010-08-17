#	$NetBSD: Makefile,v 1.30.2.1 2010/08/17 06:44:10 uebayasi Exp $

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

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
