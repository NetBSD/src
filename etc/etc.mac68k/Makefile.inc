#	$NetBSD: Makefile.inc,v 1.8 2001/11/29 22:45:56 thorpej Exp $
#
#	mac68k-specific release building goo
#

# You will find no configs for GENERICSBC or INSTALLSBC in the
# sources. That's because they're trivial to generate--each is
# simply its corresponding config with "ncrscsi" commented out,
# and "sbc" uncommented.
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

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
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} ${MAKEFLAGS} INSTALL_KERNEL=${k} && \
		${MAKE} ${MAKEFLAGS} INSTALL_KERNEL=${k} release
.endfor
	cd ${KERNOBJDIR}/GENERICSBC &&	\
		tar cf - netbsd |\
		gzip -c -9 > ${RELEASEDIR}/binary/sets/kern_sbc.tgz
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/notes && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/notes && \
		${MAKE} ${MAKEFLAGS} && \
		${MAKE} ${MAKEFLAGS} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
