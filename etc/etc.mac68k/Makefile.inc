#	$NetBSD: Makefile.inc,v 1.5 2000/05/31 15:45:55 fredb Exp $
#
#	mac68k-specific release building goo
#

# You will find no configs for GENERICSBC or INSTALLSBC in the
# sources. That's because they're trivial to generate--each is
# simply its corresponding config with "ncrscsi" commented out,
# and "sbc" uncommented.
#
BUILD_KERNELS?=	INSTALL	# INSTALLSBC
EXTRA_KERNELS?=		# GENERICSBC

INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && ${MAKE}
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && ${MAKE} clean
.endif
.for k in ${BUILD_KERNELS}
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} INSTALL_KERNEL=${k} && ${MAKE} release INSTALL_KERNEL=${k}
.endfor
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
