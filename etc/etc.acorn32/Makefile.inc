#	$NetBSD: Makefile.inc,v 1.11 2006/08/16 19:33:52 he Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC NC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel	\
			installation/misc

snap_md_post:
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot32,ffa \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.gz' boot32,ffa
