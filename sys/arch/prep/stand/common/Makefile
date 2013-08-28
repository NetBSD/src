#	$NetBSD: Makefile,v 1.10.22.1 2013/08/28 23:59:20 rmind Exp $

S= ${.CURDIR}/../../../..

NOMAN= # defined

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -D__daddr_t=int32_t
CPPFLAGS+= -I. -I${S} -I${.OBJDIR}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes

CFLAGS= -Os -mmultiple -ffreestanding

.include <bsd.prog.mk>
.include <bsd.klinks.mk>

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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes SA_INCLUDE_NET=no
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

LIBS= ${LIBSA} ${LIBZ} ${LIBKERN}

realall: ${LIBS}

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
