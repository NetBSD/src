#	$NetBSD: Makefile,v 1.46 2018/08/07 08:26:42 martin Exp $
#

.include <bsd.own.mk>

TARGETS+=	release

.ifndef ALLNOTES
.if exists(${RELEASEMACHINE}/Makefile)
SUBDIR=	${RELEASEMACHINE}
.endif
.else
SUBDIR+= acorn32
SUBDIR+= alpha
SUBDIR+= amd64
SUBDIR+= amiga
SUBDIR+= arc
SUBDIR+= atari
SUBDIR+= bebox
SUBDIR+= cats
SUBDIR+= emips
SUBDIR+= evbarm
SUBDIR+= evbppc
SUBDIR+= hp300
SUBDIR+= hppa
SUBDIR+= hpcmips
SUBDIR+= hpcsh
SUBDIR+= i386
# SUBDIR+= iyonix	# not yet
SUBDIR+= landisk
SUBDIR+= mac68k
SUBDIR+= macppc
SUBDIR+= mmeye
SUBDIR+= mvme68k
SUBDIR+= news68k
SUBDIR+= newsmips
SUBDIR+= next68k
SUBDIR+= ofppc
SUBDIR+= pmax
SUBDIR+= prep
SUBDIR+= sandpoint
SUBDIR+= sgimips
SUBDIR+= shark
SUBDIR+= sparc
SUBDIR+= sparc64
SUBDIR+= sun2
SUBDIR+= sun3
SUBDIR+= vax
SUBDIR+= x68k
.endif

.include <bsd.subdir.mk>
