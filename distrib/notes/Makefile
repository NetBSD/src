#	$NetBSD: Makefile,v 1.30 2002/06/29 09:39:44 lukem Exp $
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
# SUBDIR+= arc		# not complete
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
