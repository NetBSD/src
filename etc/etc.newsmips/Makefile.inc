#	$NetBSD: Makefile.inc,v 1.5 2006/01/22 14:44:16 tsutsui Exp $
#
#	etc.newsmips/Makefile.inc -- newsmips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
