#	$NetBSD: Makefile.inc,v 1.20 2002/12/02 13:31:17 lukem Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot '*.gz'
