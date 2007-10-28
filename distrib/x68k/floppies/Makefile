#	$NetBSD: Makefile,v 1.11 2007/10/28 13:59:53 isaki Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT bootfloppy.sysinst
# `floppy-root' cannot be used now, due to size issue.
#SUBDIR+=	bootfloppy.generic

TARGETS+=	release

.include <bsd.subdir.mk>
