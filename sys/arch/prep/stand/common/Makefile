#	$NetBSD: Makefile,v 1.4 2003/11/27 16:19:16 tsutsui Exp $

S= ${.CURDIR}/../../../..

NOMAN= # defined

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -I. -I${S} -I${.OBJDIR}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes

CFLAGS+= -ffreestanding

CLEANFILES+= machine powerpc

.PHONY: machine-links
beforedepend: machine-links

machine-links:
	-rm -f machine
	ln -s ${S}/arch/${MACHINE}/include machine
	-rm -f ${MACHINE_ARCH}
	ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}

.include <bsd.prog.mk>

### find out what to use for libkern
KERN_AS=	library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

### find out what to use for libz
Z_AS=		library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=		${ZLIB}

### find out what to use for libsa
SA_AS=		library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

LIBS= ${LIBSA} ${LIBZ} ${LIBKERN}

realall: machine-links ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
