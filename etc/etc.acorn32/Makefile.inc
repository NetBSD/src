#	$NetBSD: Makefile.inc,v 1.13 2008/03/18 04:20:36 lukem Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC NC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel	\
			installation/misc

snap_md_post:
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot32,ffa \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.gz' boot32,ffa
