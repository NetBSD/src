#	$NetBSD: Makefile,v 1.5 1999/03/04 19:46:09 drochner Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes.
#
# Eventually, we should install everything.

SUBDIR=	${MACHINE}
.if ${MACHINE} != ${MACHINE_ARCH}
ARCHSUBDIR= ${MACHINE_ARCH:C/mipse[bl]/mips/}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif

#SUBDIR= alpha amiga arm32 atari bebox hp300 i386 m68k mac68k macppc mips \
#	mvme68k newsmips ofppc pc532 pica pmax powerpc sparc sun3 vax x68k

SYMLINKS= ${MACHINE} /usr/include/machine

SYMLINKS+= machine/float.h /usr/include/float.h \
	machine/frame.h /usr/include/frame.h \
	machine/stdarg.h /usr/include/stdarg.h \
	machine/varargs.h /usr/include/varargs.h

.include <bsd.kinc.mk>
