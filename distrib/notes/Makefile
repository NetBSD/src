#	$NetBSD: Makefile,v 1.21 2002/03/30 07:00:04 thorpej Exp $
#

TARGETS+= release

.ifndef ALLNOTES
.if exists(${MACHINE}/Makefile)
SUBDIR=	${MACHINE}
.endif
.else
SUBDIR+= acorn26
SUBDIR+= acorn32
SUBDIR+= alpha
SUBDIR+= amiga
SUBDIR+= arc
SUBDIR+= atari
SUBDIR+= evbarm
SUBDIR+= hp300
SUBDIR+= hpcmips
SUBDIR+= i386
SUBDIR+= mac68k
SUBDIR+= macppc
SUBDIR+= mvme68k
SUBDIR+= news68k
SUBDIR+= next68k
SUBDIR+= pc532
SUBDIR+= pmax
SUBDIR+= sandpoint
SUBDIR+= sparc
SUBDIR+= sparc64
SUBDIR+= sun2
SUBDIR+= sun3
SUBDIR+= vax
SUBDIR+= x68k
.endif

.include <bsd.subdir.mk>
