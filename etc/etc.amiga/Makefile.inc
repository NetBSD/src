#	$NetBSD: Makefile.inc,v 1.15 2001/11/29 22:45:54 thorpej Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL INSTWDCPCM

INSTALLATION_DIRS+=	installation/miniroot \
			installation/misc
  
snap_md_post:
#	(cd $(SRCTOP)/distrib/miniroot; ${MAKE} release)
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	sh ${.CURDIR}/../distrib/sets/makesums -t ${RELEASEDIR}/installation/miniroot '*.gz'
