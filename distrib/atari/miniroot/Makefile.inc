#	$NetBSD: Makefile.inc,v 1.10 2002/05/10 21:11:48 leo Exp $

IMAGESIZE=	1440k
IMAGEENDIAN=	be
MAKEFS_FLAGS=	-o density=3k
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt

# add cut-down gethostby*() from libhack
#
HACKOBJS=	gethost.o
