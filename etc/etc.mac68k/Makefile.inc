#	$NetBSD: Makefile.inc,v 1.4.2.1 2000/06/23 15:46:00 minoura Exp $
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
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && ${MAKE} ${MAKEFLAGS} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && ${MAKE} ${MAKEFLAGS}
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && ${MAKE} ${MAKEFLAGS} clean
.endif
.for k in ${BUILD_KERNELS}
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} ${MAKEFLAGS} INSTALL_KERNEL=${k} && ${MAKE} ${MAKEFLAGS} release INSTALL_KERNEL=${k}
.endfor
	cd ${.CURDIR}/../distrib/notes && ${MAKE} ${MAKEFLAGS} && ${MAKE} ${MAKEFLAGS} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
