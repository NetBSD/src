#	$NetBSD: Makefile.inc,v 1.22 2004/06/01 03:08:42 mhitch Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
	cd ${KERNSRCDIR}/arch/amiga/stand/misc && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.gz'
