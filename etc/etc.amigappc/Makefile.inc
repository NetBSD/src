#	$NetBSD: Makefile.inc,v 1.2 2006/01/14 23:05:28 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=			

EXTRA_KERNELS=	

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/miniroot '*.gz'
