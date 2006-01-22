#	$NetBSD: Makefile.inc,v 1.7 2006/01/22 14:42:34 tsutsui Exp $
#
#	etc.news68k/Makefile.inc -- news68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_TINY

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
