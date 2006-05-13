#	$NetBSD: Makefile,v 1.2.8.1 2006/05/13 16:54:13 tron Exp $

IMAGE=		prepare.fs
IMAGESIZE?=	1080k
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	install.sh install.md ${.CURDIR}/../../../miniroot/install.sub
PARSELISTENV=	DISTRIBREV=${DISTRIBREV:Q}

.include "../common/Makefile.images"
