#	$NetBSD: Makefile.inc,v 1.7 2012/01/29 10:36:33 tsutsui Exp $
#
#	etc.zaurus/Makefile.inc -- zaurus-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SETS+=		C700

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

# zaurus specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/zaurus/stand/zboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/zaurus/stand/zbsdmod && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation '*'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*'
