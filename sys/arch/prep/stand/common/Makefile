#	$NetBSD: Makefile,v 1.1 2000/02/29 15:21:52 nonaka Exp $

S= ${.CURDIR}/../../../..

MKMAN= no

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -I. -I${S} -I${.OBJDIR}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes

CLEANFILES+= machine powerpc

.BEGIN: machine powerpc
.NOPATH: machine powerpc
realdepend realall: machine powerpc

machine::
	-rm -f $@
	ln -s ${S}/arch/${MACHINE}/include $@

powerpc::
	-rm -f $@
	ln -s ${S}/arch/powerpc/include $@

${OBJS}: machine powerpc

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

realall: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

.include <bsd.prog.mk>
