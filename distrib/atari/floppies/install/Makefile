#	$NetBSD: Makefile,v 1.6.14.1 2020/04/13 07:45:25 martin Exp $

IMAGE=		sysinst.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
USE_SYSINST=	yes
IMGMAKEFSOPTIONS= -o bsize=4096,fsize=512,density=4096

HACK_CURSES=yes
.include "../common/Makefile.images"
