#	$NetBSD: Makefile.inc,v 1.10 2003/08/02 18:02:36 bjh21 Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC NC NC_WSCONS RPC_WSCONS

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel	\
			installation/misc

snap_md_post:
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot32,ffa \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.gz' boot32,ffa
