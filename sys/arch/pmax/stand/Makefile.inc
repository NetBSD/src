#	$NetBSD: Makefile.inc,v 1.9 1999/03/26 02:12:17 simonb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

BINDIR?=/usr/mdec
BINMODE?=444

.BEGIN: machine
depend all ${SRCS} ${OBJS}: machine
CLEANFILES+= machine mips

# Do `mips' link here too - can't just depend on target mips
# because make finds sys/lib/libkern/mips...
machine:
	-rm -f ${.TARGET} mips
	ln -s $S/arch/${MACHINE}/include ${.TARGET}
	ln -s $S/arch/mips/include mips

# Tailor C compilation for standalone environment.
COPTS=	-Os				# -Os gives smaller code

C_MACHDEP=-mmemcpy -mno-abicalls -G 128	# even smaller code

DEFS?=	-DSMALL -D_STANDALONE -D_NO_PROM_DEFINES -DNO_ABICALLS \
	${BOOTDEFADD}
INCL?=	-nostdinc -I${.OBJDIR} -I$S -I${PMAX_STAND_DIR}/lib -I$S/lib/libsa

CFLAGS+=	${CWARN} ${C_MACHDEP}
AFLAGS+=	-D_LOCORE -D_KERNEL
CPPFLAGS+=	${DEFS} ${INCL}

# be sure to turn off any PIC flags for standalone library code.
CPICFLAGS=
CAPICFLAGS=
CPPPICFLAGS=
APICFLAGS=
