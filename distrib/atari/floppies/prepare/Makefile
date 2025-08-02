#	$NetBSD: Makefile,v 1.3.100.1 2025/08/02 05:20:19 perseant Exp $

IMAGE=		prepare.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	install.sh install.md ${.CURDIR}/../../../miniroot/install.sub
PARSELISTENV=	DISTRIBREV=${DISTRIBREV:Q}

.include "../common/Makefile.images"
