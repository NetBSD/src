#	$NetBSD: Makefile.inc,v 1.14 2000/09/17 19:04:39 is Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+=INSTALL INSTWDCPCM

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
