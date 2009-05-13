#	$NetBSD: Makefile,v 1.10.2.1 2009/05/13 17:18:10 jym Exp $

S= ${.CURDIR}/../../../..

NOMAN= # defined

CFLAGS= -Os -Wall
CFLAGS+= -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I.
CPPFLAGS+= -D__daddr_t=int32_t
AFLAGS= -D_LOCORE

.include <bsd.klinks.mk>
.include <bsd.prog.mk>

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libz
Z_AS= library
.include "${S}/lib/libz/Makefile.inc"
LIBZ= ${ZLIB}

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes SA_EXTRADIR=${.CURDIR}
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

LIBS= ${LIBSA} ${LIBZ} ${LIBKERN}

realall: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
