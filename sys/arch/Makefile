#	$NetBSD: Makefile,v 1.8 2001/01/04 08:32:02 itojun Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes.
#
# Eventually, we should install everything.

SUBDIR=	${MACHINE}
.if ${MACHINE} != ${MACHINE_ARCH}
ARCHSUBDIR= ${MACHINE_ARCH:C/mipse[bl]/mips/:C/sh3e[bl]/sh3/}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif
.if ${MACHINE} == sparc
SUBDIR+= sparc64
.endif

#SUBDIR= alpha amiga arc arm32 atari bebox evbsh3 hp300 hpcmips i386 luna68k \
#	m68k mac68k macppc mips mmeye mvme68k news68k newsmips next68k ofppc \
#	pc532 pmax powerpc sh3 sparc sparc64 sun3 vax x68k

SYMLINKS= ${MACHINE} /usr/include/machine

SYMLINKS+= machine/float.h /usr/include/float.h \
	machine/frame.h /usr/include/frame.h \
	machine/stdarg.h /usr/include/stdarg.h \
	machine/varargs.h /usr/include/varargs.h

.include <bsd.kinc.mk>
