#	$NetBSD: Makefile.inc,v 1.10 2002/02/01 04:57:24 fredb Exp $
#
#	mac68k-specific release building goo
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
		GZIP=-9 ${PAX} -zwf	\
		    ${RELEASEDIR}/binary/sets/kern_sbc.tgz netbsd
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/notes && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/notes && \
		${MAKE} ${MAKEFLAGS} && \
		${MAKE} ${MAKEFLAGS} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
