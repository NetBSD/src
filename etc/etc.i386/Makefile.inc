#	$NetBSD: Makefile.inc,v 1.17 2000/03/26 15:48:22 fredb Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= GENERIC_TINY

# These are for use by the bootfloppy construction tools
BUILD_KERNELS+= INSTALL INSTALL_TINY

INSTALLATION_DIRS+=	${RELEASEDIR}/binary/sets/Split

snap_md_post:
# Build the boot floppies and install them
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/i386/floppies && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/i386/floppies && \
		${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppies '*.gz'
# Make the split sets in place
	cd ${RELEASEDIR}/binary/sets/Split; for tarfile in ../*.tgz; do \
		split -b235k $$tarfile `basename $$tarfile tgz`; done
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/binary/sets/Split
