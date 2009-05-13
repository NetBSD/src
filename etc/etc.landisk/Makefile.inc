# $NetBSD: Makefile.inc,v 1.2.22.1 2009/05/13 19:18:02 jym Exp $
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
