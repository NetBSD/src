#	$NetBSD: Makefile,v 1.27 2011/10/11 13:07:06 tsutsui Exp $

S=		${.CURDIR}/../../../..

CFLAGS+=	-m68020-60

CPPFLAGS+=	-nostdinc -I${.OBJDIR} -I. -I${S}
CPPFLAGS+=	-I${.CURDIR}/../libiocs -I${.CURDIR}/../common
CPPFLAGS+=	-D_STANDALONE
CPPFLAGS+=	-DHEAP_VARIABLE
CPPFLAGS+=	-DHAVE_CHANGEDISK_HOOK
CPPFLAGS+=	-DUSTAR_SECT_PER_CYL=16
#CPPFLAGS+=	-DDEBUG

.PATH:	${LIBSADIR} ${LIBKERNDIR} ${LIBZDIR} ${LIBZDIST}

.include "../Makefile.booters"

# only needed during build
libinstall::

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

LIBS= ${LIBKERN} ${LIBZ} ${LIBSA}

all realall: ${LIBS}

cleandir distclean: .WAIT cleanlibdir

cleanlibdir:
	-rm -rf lib
