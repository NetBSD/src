#	$NetBSD: Makefile,v 1.12 1999/06/30 18:23:02 ross Exp $

TARGETS += release

.ifndef ALLNOTES
SUBDIR = ${MACHINE}
.else
SUBDIR = alpha amiga arm32 atari hp300 i386 mac68k
SUBDIR+= mvme68k pc532 pmax sparc sun3 vax x68k
.endif

.include <bsd.subdir.mk>
