#	$NetBSD: Makefile.inc,v 1.24 2004/11/21 21:07:15 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL ISDN

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.gz'
