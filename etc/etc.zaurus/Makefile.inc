#	$NetBSD: Makefile.inc,v 1.6 2009/03/05 10:28:02 nonaka Exp $
#
#	etc.zaurus/Makefile.inc -- zaurus-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

# zaurus specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/zaurus/stand/zboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/zaurus/stand/zbsdmod && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation '*'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*'
