#	$NetBSD: Makefile,v 1.5 2002/04/13 17:48:10 tsutsui Exp $

S= ${.CURDIR}/../../../..

NOMAN= # defined

CFLAGS= -Os -Wall

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -DLIBSA_USE_MEMCPY -DLIBSA_USE_MEMSET
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I.
AFLAGS= -D_LOCORE

CLEANFILES+= machine m68k

.BEGIN: machine m68k
.NOPATH: machine m68k
realdepend realall: machine m68k

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

${OBJS}: machine m68k

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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

LIBS= ${LIBSA} ${LIBZ} ${LIBKERN}

realall: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
