#	$NetBSD: Makefile,v 1.10.2.1 2000/03/29 22:38:45 he Exp $

TARGETS+=release

.ifndef ALLNOTES
SUBDIR = ${MACHINE}
.else
SUBDIR = alpha amiga arm32 atari hp300 i386 mac68k
SUBDIR+= mvme68k pc532 pmax sparc sun3 sun3x vax x68k
.endif

.include <bsd.subdir.mk>
