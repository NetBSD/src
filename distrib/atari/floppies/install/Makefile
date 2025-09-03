#	$NetBSD: Makefile,v 1.9 2025/06/29 10:10:32 martin Exp $

IMAGE=		sysinst.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
USE_SYSINST=	yes
IMGMAKEFSOPTIONS= -o bsize=4096,fsize=512,density=4096

.include "../common/Makefile.images"
