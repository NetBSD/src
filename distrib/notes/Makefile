#	$NetBSD: Makefile,v 1.14 2000/04/07 17:33:34 thorpej Exp $

TARGETS+= release

.ifndef ALLNOTES
.if exists(${MACHINE}/Makefile)
SUBDIR=	${MACHINE}
.endif
.else
SUBDIR=	alpha amiga arm32 atari hp300 i386 mac68k
SUBDIR+=mvme68k pc532 pmax sparc sun3 vax x68k
.endif

.include <bsd.subdir.mk>
