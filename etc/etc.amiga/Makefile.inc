#	$NetBSD: Makefile.inc,v 1.21 2003/01/03 15:34:37 lukem Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.gz'
