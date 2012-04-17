#	$NetBSD: Makefile.inc,v 1.6.6.1 2012/04/17 00:02:58 yamt Exp $
#
#	etc.zaurus/Makefile.inc -- zaurus-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SETS+=		C700

BUILD_KERNELS=		INSTALL
BUILD_KERNELS+=		INSTALL_C700

INSTALLATION_DIRS+=	installation/kernel

# zaurus specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/zaurus/stand/zboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/zaurus/stand/zbsdmod && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation '*'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*'
