#	$NetBSD: Makefile,v 1.7 2019/07/28 10:28:49 martin Exp $

IMAGE=		sysinst.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
USE_SYSINST=	yes
IMGMAKEFSOPTIONS= -o bsize=4096,fsize=512,density=4096

HACK_CURSES=yes
.include "../common/Makefile.images"
