#	$NetBSD: Makefile,v 1.17 2001/11/30 22:20:03 jhawk Exp $
#

TARGETS+= release

.ifndef ALLNOTES
.if exists(${MACHINE}/Makefile)
SUBDIR=	${MACHINE}
.endif
.else
SUBDIR=	alpha amiga arm26 arm32 atari hp300 hpcmips i386
SUBDIR+=mac68k macppc mvme68k news68k next68k pc532 pmax sparc
SUBDIR+=sparc64 sun3 vax x68k
.endif

.include <bsd.subdir.mk>
