#	$NetBSD: Makefile.inc,v 1.4 1999/01/29 05:51:15 simonb Exp $
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
COPTS=	-O2		# -O2 gives smaller code

C_MACHDEP=-mno-abicalls -G 64	# may give smaller code

# We are so tight for space, ifdef away symlnks and spinning propellers.
DEFS?=	-D_STANDALONE -DSMALL -DNO_TWIDDLE
INCL?=	-I. -I${.CURDIR} -I$S -I${.CURDIR}/../libsa -I$S/lib/libsa

CFLAGS=	${COPTS} ${CWARN} ${INCPATH} ${DEFS} ${C_MACHDEP}
AFLAGS=	-O2 ${INCPATH} ${DEFS} -D_LOCORE -D_KERNEL
CPPFLAGS= ${INCL}

# be sure to turn off any PIC flags for standalone library code.
CPICFLAGS=
CAPICFLAGS=
CPPPICFLAGS=
APICFLAGS=
