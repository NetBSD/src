#	$NetBSD: Makefile.inc,v 1.1 2001/11/30 14:22:33 reinoud Exp $
#
#	etc.arm32/Makefile.inc -- arm32-specific etc Makefile targets
#

KERNEL_SETS=	RISCPC RPC_WSCONS
BUILD_KERNELS=	RPCINST

INSTALLATION_DIRS+=	installation/kernel \
			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/arm32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/kernels '*.gz'
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc '*.gz'
