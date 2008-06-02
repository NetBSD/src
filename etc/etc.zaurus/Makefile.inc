#	$NetBSD: Makefile.inc,v 1.2.10.1 2008/06/02 13:21:20 mjf Exp $
#
#	etc.zaurus/Makefile.inc -- zaurus-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

# zaurus specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/zaurus/stand/zbsdmod && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation '*'
