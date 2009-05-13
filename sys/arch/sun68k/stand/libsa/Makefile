#	$NetBSD: Makefile,v 1.25.2.1 2009/05/13 17:18:40 jym Exp $

# Logically src/sys
S!= cd ${.CURDIR}/../../../..; pwd
DIR_SA=${S}/lib/libsa
DIR_KERN=${S}/lib/libkern ${S}/lib/libkern/arch/m68k
DIR_LIBC=${S}/../common/lib/libc

# DEBUG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
#  -DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG -DDEBUG_PROM

CPUFLAGS=	# ignore settings in /etc/mk.conf
DEFS= -Dsun3 -D_STANDALONE -D__daddr_t=int32_t
INCL= -I. -I${.CURDIR} -I${S}/lib/libsa -I${S} -I${S}/arch
AFLAGS= -Wa,-mc68020 -Wa,-mc68851
CFLAGS= -mc68000 -Wa,-mc68010 -Wa,-m68851 -msoft-float
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith
COPTS= -Os -fno-defer-pop -ffreestanding
CPPFLAGS= ${DEFS} ${DEBUG} ${INCL}
CLEANFILES= SRT0.o SRT1.o vers.c

SRCS= SRT0.S SRT1.c
OBJS= SRT0.o SRT1.o

realall: ${OBJS}

.include <bsd.prog.mk>

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS= SA_USE_LOADFILE=yes SA_EXTRADIR=${.CURDIR}
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

LIBS= ${LIBSA} ${LIBKERN}

realall: ${LIBS}

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
