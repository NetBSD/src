#	$NetBSD: Makefile.inc,v 1.3 2000/02/20 03:52:22 mycroft Exp $
#
#	mac68k-specific release building goo
#

# This is commented out in the distribution, because GENERICSBC isn't
# distributed (it's just GENERIC:s/ncrsci/sbc/).
#
#EXTRA_KERNELS+=	GENERICSBC
BUILD_KERNELS+= 	INSTALL

INSTALLATION_DIRS+=		installation/instkernel

snap_md_post:
#
# Build the ramdisk...
#
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && \
		${MAKE} ${_J}
#
# and put them all together.
#
.for k in ${EXTRA_INSTALL_KERNELS}
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} INSTALL_KERNEL=${k} && ${MAKE} release INSTALL_KERNEL=${k}
.endfor
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
