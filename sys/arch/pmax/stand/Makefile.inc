#	$NetBSD: Makefile.inc,v 1.2 1999/01/21 14:16:52 simonb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

BINDIR=	/usr/mdec
STRIPFLAG=
BINMODE=444

# Tailor C compilation for standalone environment.
COPTS=	-O2		# -O2 gives smaller code
CWARN=	-Werror

C_MACHDEP= -mno-abicalls -mno-half-pic -fno-PIC -G 64	# may give smaller code
AS-=	-KPIC

# We are so tight for space, ifdef away symlnks and spinning propellers.
BOOTDEFS=-D_STANDALONE -DSMALL -DNO_TWIDDLE
DEFS=	-Dpmax ${BOOTDEFS}

CFLAGS=	${COPTS} ${CWARN} ${INCPATH} ${DEFS} ${C_MACHDEP}
AFLAGS=	-O2 ${INCPATH} ${DEFS} -D_LOCORE

# be sure to turn off any PIC flags for standalone library code.

CPICFLAGS=
CAPICFLAGS=
CPPPICFLAGS=
CPPPPICFLAGS=
APICFLAGS=

CPPFLAGS+= -I$S -I${.CURDIR}/../lib -I$S/lib/libsa
