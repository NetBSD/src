#	$NetBSD: Makefile,v 1.22 2002/04/02 12:35:43 skrll Exp $
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
SUBDIR+= atari
SUBDIR+= cats
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
SUBDIR+= shark
SUBDIR+= sparc
SUBDIR+= sparc64
SUBDIR+= sun2
SUBDIR+= sun3
SUBDIR+= vax
SUBDIR+= x68k
.endif

.include <bsd.subdir.mk>
