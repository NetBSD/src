#	$NetBSD: Makefile.inc,v 1.7 1999/02/23 01:23:26 simonb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

BINDIR=	/usr/mdec
BINMODE=444

beforedepend ${OBJS}: machine
CLEANFILES+=machine

machine:
	-rm -f ${.TARGET}
	ln -s $S/arch/pmax/include ${.TARGET}

# Tailor C compilation for standalone environment.
COPTS=	-Os				# -Os gives smaller code

C_MACHDEP=-mmemcpy -mno-abicalls -G 128	# even smaller code

# We are reasonably tight for space, ifdef away symlnks
DEFS?=	-DSMALL -D_STANDALONE -D_NO_PROM_DEFINES -DUFS_NOCLOSE -DUFS_NOSYMLINK \
	-DSA_NOPUTCHAR -DLIBSA_USE_MEMCPY -DLIBSA_USE_MEMSET
INCL?=	-I. -I${.CURDIR} -I$S -I${.CURDIR}/../libsa -I$S/lib/libsa

CFLAGS=	${COPTS} ${CWARN} ${INCPATH} ${DEFS} ${C_MACHDEP}
AFLAGS=	-O2 ${INCPATH} ${DEFS} -D_LOCORE -D_KERNEL
CPPFLAGS= ${INCL}

# be sure to turn off any PIC flags for standalone library code.
CPICFLAGS=
CAPICFLAGS=
CPPPICFLAGS=
APICFLAGS=
