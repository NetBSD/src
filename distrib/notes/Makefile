#	$NetBSD: Makefile,v 1.18 2001/12/17 21:39:51 fredette Exp $
#

TARGETS+= release

.ifndef ALLNOTES
.if exists(${MACHINE}/Makefile)
SUBDIR=	${MACHINE}
.endif
.else
SUBDIR=	alpha amiga arm26 arm32 atari hp300 hpcmips i386
SUBDIR+=mac68k macppc mvme68k news68k next68k pc532 pmax sparc
SUBDIR+=sparc64 sun2 sun3 vax x68k
.endif

.include <bsd.subdir.mk>
