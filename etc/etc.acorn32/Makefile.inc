#	$NetBSD: Makefile.inc,v 1.9 2003/01/03 15:34:37 lukem Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC NC NC_WSCONS RPC_WSCONS

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.gz'
