#	$NetBSD: Makefile.inc,v 1.8 2002/12/02 13:31:16 lukem Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC NC NC_WSCONS RPC_WSCONS

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
