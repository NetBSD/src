#	$NetBSD: Makefile.inc,v 1.9 2002/05/07 14:59:35 lukem Exp $

IMAGESIZE=	1080k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt

# add cut-down gethostby*() from libhack
#
HACKOBJS=	gethost.o
