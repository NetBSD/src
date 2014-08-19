#	$NetBSD: Makefile,v 1.2.18.1 2014/08/20 00:03:21 tls Exp $

S= ${.CURDIR}/../../../..

.include <bsd.klinks.mk>

NOMAN= # defined

CPPFLAGS+= -D_STANDALONE
CPPFLAGS+= -D__daddr_t=int32_t
CPPFLAGS+= -I. -I${S} -I${.OBJDIR}
CPPFLAGS+= -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes -Wno-pointer-sign

CFLAGS= -Os -ffreestanding

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
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes SA_INCLUDE_NET=no
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=		${SALIB}

LIBS= ${LIBSA} ${LIBZ} ${LIBKERN}

realall: ${LIBS}
