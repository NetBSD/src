#	$NetBSD: Makefile,v 1.1 1998/06/12 23:22:31 cgd Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes.
#
# Eventually, we should install everything.

SUBDIR=	${MACHINE}
.if ${MACHINE} != ${MACHINE_ARCH}
SUBDIR+= ${MACHINE_ARCH}
.endif

#SUBDIR= alpha amiga arm32 atari bebox hp300 i386 m68k mac68k macppc mips \
#	mvme68k newsmips ofppc pc532 pica pmax powerpc sparc sun3 vax x68k

SYMLINKS= ${MACHINE} /usr/include/machine

.include <bsd.kinc.mk>
