#	$NetBSD: Makefile.inc,v 1.8 2000/05/31 21:09:48 matt Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

# 
# We need this to build the sysinst kernel.
BUILD_KERNELS+=INSTALL

distribution: pcs750.bin

pcs750.bin: etc.vax/pcs750.bin.uu
	(cd etc.vax; uudecode pcs750.bin.uu)
	(cd etc.vax; install -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)
	

INSTALLATION_DIRS+=	installation \
			installation/bootfs \
			installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/vax && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
