#	$NetBSD: Makefile,v 1.14 2001/10/13 14:46:00 bjh21 Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes (usually).
#
# Eventually, we should install everything.

SUBDIR=	${MACHINE}
ARCHSUBDIR= ${MACHINE_ARCH:C/mipse[bl]/mips/:C/sh3e[bl]/sh3/:S/arm26/arm/:S/arm32/arm/:S/m68000/m68k/}
.if ${MACHINE} != ${ARCHSUBDIR}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif
.if ${MACHINE} == sparc
SUBDIR+= sparc64
.endif
.if (${MACHINE} == hpcmips || ${MACHINE} == hpcsh)
SUBDIR+= hpc
.endif
.if (${MACHINE} == sun2)
SUBDIR+= sun68k
.endif

.if ${ARCHSUBDIR} == arm && ${MACHINE} != acorn32 && ${MACHINE} != arm32 && \
	${MACHINE} != arm26
SUBDIR+= cats dnard netwinder
.endif

#SUBDIR= alpha amiga arc arm32 atari bebox evbsh3 hp300 hpcmips i386 luna68k \
#	m68k mac68k macppc mips mmeye mvme68k news68k newsmips next68k ofppc \
#	pc532 pmax powerpc sh3 sparc sparc64 sun3 vax x68k

#
# For true arm ports, we point machine at the architecture directory
# since all the user include are there and are common.
#
.if ${ARCHSUBDIR} == arm && ${MACHINE} != acorn32 && ${MACHINE} != arm32 && \
	${MACHINE} != arm26
SYMLINKS= ${ARCHSUBDIR} /usr/include/machine
.else
SYMLINKS= ${MACHINE} /usr/include/machine
.endif

SYMLINKS+= machine/float.h /usr/include/float.h \
	machine/frame.h /usr/include/frame.h \
	machine/stdarg.h /usr/include/stdarg.h \
	machine/varargs.h /usr/include/varargs.h

.include <bsd.kinc.mk>
