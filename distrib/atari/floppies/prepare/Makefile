#	$NetBSD: Makefile,v 1.3 2006/05/12 05:50:30 jdc Exp $

IMAGE=		prepare.fs
IMAGESIZE?=	1080k
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	install.sh install.md ${.CURDIR}/../../../miniroot/install.sub
PARSELISTENV=	DISTRIBREV=${DISTRIBREV:Q}

.include "../common/Makefile.images"
