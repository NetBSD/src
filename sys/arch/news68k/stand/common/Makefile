#	$NetBSD: Makefile,v 1.1.2.1 1999/12/27 18:33:04 wrstuden Exp $

S= ${.CURDIR}/../../../..

MKMAN= no

CFLAGS= -Os -Wall

CPPFLAGS+= -D_STANDALONE -DBOOT_DEBUG
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S} -I.
AFLAGS= -D_LOCORE

CLEANFILES+= ${.OBJDIR}/machine ${.OBJDIR}/m68k

.BEGIN: ${.OBJDIR}/machine ${.OBJDIR}/m68k

${.OBJDIR}/machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

${.OBJDIR}/m68k::
	-rm -f $@
	ln -s ${S}/arch/m68k/include $@

depend all: ${.OBJDIR}/machine ${.OBJDIR}/m68k

${OBJS}: ${.OBJDIR}/machine ${.OBJDIR}/m68k

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

all: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
