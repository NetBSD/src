#	$NetBSD: Makefile,v 1.20 2002/03/24 22:14:23 bjh21 Exp $
#

TARGETS+= release

.ifndef ALLNOTES
.if exists(${MACHINE}/Makefile)
SUBDIR=	${MACHINE}
.endif
.else
SUBDIR=	alpha amiga acorn26 acorn32 atari hp300 hpcmips i386
SUBDIR+=mac68k macppc mvme68k news68k next68k pc532 pmax sparc
SUBDIR+=sparc64 sun2 sun3 vax x68k
.endif

.include <bsd.subdir.mk>
