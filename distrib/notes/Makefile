#	$NetBSD: Makefile,v 1.13 2000/03/21 03:58:12 garbled Exp $

TARGETS += release

.ifndef ALLNOTES
.if exists($MACHINE)
SUBDIR = ${MACHINE}
.endif
.else
SUBDIR = alpha amiga arm32 atari hp300 i386 mac68k
SUBDIR+= mvme68k pc532 pmax sparc sun3 vax x68k
.endif

.include <bsd.subdir.mk>
