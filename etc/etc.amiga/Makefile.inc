#	$NetBSD: Makefile.inc,v 1.19 2002/12/02 13:18:06 lukem Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc
  
snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot '*.gz'
