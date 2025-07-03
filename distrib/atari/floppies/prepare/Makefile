#	$NetBSD: Makefile,v 1.4 2025/07/03 11:10:21 martin Exp $

IMAGE=		prepare.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	install.sh install.md ${.CURDIR}/../../../miniroot/install.sub
PARSELISTENV=	DISTRIBREV=${DISTRIBREV:Q}

.include "../common/Makefile.images"
