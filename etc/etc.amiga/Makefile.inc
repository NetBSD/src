#	$NetBSD: Makefile.inc,v 1.16 2002/02/10 20:43:34 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL

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
