#	$NetBSD: Makefile,v 1.24.2.2 2002/06/19 01:05:48 lukem Exp $
#

TARGETS+=	release

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
SUBDIR+= arm32
SUBDIR+= atari
SUBDIR+= cats
SUBDIR+= evbarm
SUBDIR+= hp300
SUBDIR+= hpcmips
SUBDIR+= hpcsh
SUBDIR+= i386
SUBDIR+= mac68k
SUBDIR+= macppc
SUBDIR+= mmeye
SUBDIR+= mvme68k
SUBDIR+= news68k
SUBDIR+= newsmips
SUBDIR+= next68k
SUBDIR+= pc532
SUBDIR+= pmax
SUBDIR+= pmppc
SUBDIR+= sandpoint
SUBDIR+= sgimips
SUBDIR+= shark
SUBDIR+= sparc
SUBDIR+= sparc64
SUBDIR+= sun2
SUBDIR+= sun3
SUBDIR+= vax
SUBDIR+= walnut
SUBDIR+= x68k
.endif

.include <bsd.subdir.mk>
