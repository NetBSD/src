# $NetBSD: Makefile.inc,v 1.3 2009/04/23 14:38:58 tsutsui Exp $
#
# etc.landisk/Makefile.inc -- landisk-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS=	installation/misc
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

snap_md_post:
	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/${dir} '*'
.endfor
