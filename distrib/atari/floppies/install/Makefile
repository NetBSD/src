#	$NetBSD: Makefile,v 1.8 2022/05/11 10:47:10 rin Exp $

IMAGE=		sysinst.fs
IMAGESIZE?=	1440k
MAKEDEVTARGETS=	floppy
USE_SYSINST=	yes
IMGMAKEFSOPTIONS= -o bsize=4096,fsize=512,density=4096

HACK_CURSES=yes
NOIEEE80211=1
.include "../common/Makefile.images"
