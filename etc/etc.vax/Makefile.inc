#	$NetBSD: Makefile.inc,v 1.8.2.1 2000/10/19 14:11:49 tv Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

# 
# We need this to build the sysinst kernel.
BUILD_KERNELS+=INSTALL

distribution: pcs750.bin

pcs750.bin: etc.vax/pcs750.bin.uu
	rm -f ${DESTDIR}/pcs750.bin
	uudecode -p etc.vax/pcs750.bin.uu > ${DESTDIR}/pcs750.bin
	chown ${BINOWN}.${BINGRP} ${DESTDIR}/pcs750.bin
	chmod 444 ${DESTDIR}/pcs750.bin

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
