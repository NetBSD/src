#	$NetBSD: Makefile.inc,v 1.1 1999/01/21 12:33:40 simonb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

# Tailor C compilation for standalone environment.

STANDDIR=	$S/arch/pmax/stand

XSTANDMAKE= \
	cd ${STANDDIR} && MAKEOBJDIR=${STANDDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' \
	    LD='${LD}' \
	    STANDCPPFLAGS='${CPPFLAGS:S@^-I.@-I../../.@g}'

COPTS=-O2				# -O2 gives smaller code
CWARN=-Werror
#PIC_FLAGS=-embedded-pic		# makes all code PC-rel, data GP-rel?

#C_MACHDEP=  ${PIC_FLAGS}		# fails ith binutils 2.6
C_MACHDEP= -mno-abicalls -mno-half-pic -fno-PIC -G 64	# may give smaller code
AS-=	-KPIC

# We are so tight for space, ifdef away symlnks and spinning propellers.
BOOTDEFS=-D_STANDALONE -DSMALL -DTINY -DNO_TWIDDLE
DEFS=-Dpmax ${BOOTDEFS}

CFLAGS=${COPTS} ${CWARN}  ${INCPATH} ${BOOT_TAILOR_FLAGS} ${DEFS} ${C_MACHDEP}
AFLAGS=	-O2 ${INCPATH} ${DEFS} -D_LOCORE

# be sure to turn off any PIC flags for standalone library code.

CPICFLAGS=
CAPICFLAGS=
CPPPICFLAGS=
CPPPPICFLAGS=
APICFLAGS=

### find out what to use for libkern
KERN_AS=	library
# XXX only bzero is missing in libkern, but we have to list all we need
KERNMISCMAKEFLAGS= "SRCS=bzero.S bcmp.S strchr.c strncpy.c strcmp.S __main.c"
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=	${KERNLIB}

CPPFLAGS += -I$S -I${.CURDIR}/../lib -I$S/lib/libsa
