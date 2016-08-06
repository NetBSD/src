#	$NetBSD: Makefile,v 1.6 2016/08/06 14:51:01 tsutsui Exp $

IMAGE=		sysinst.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
USE_SYSINST=	yes
IMGMAKEFSOPTIONS= -o bsize=4096,fsize=512,density=4096

.include "../common/Makefile.images"
