#	$NetBSD: Makefile.inc,v 1.6 1996/04/23 17:39:39 thorpej Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd-adp netbsd-other
#snap_md: kc_aha.fs kc_bt.fs

netbsd-adp:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICADP
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICADP && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICADP/netbsd \
	    ${DESTDIR}/snapshot/netbsd-adp

netbsd-other:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICOTHER
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICOTHER && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICOTHER/netbsd \
	    ${DESTDIR}/snapshot/netbsd-other

# XXX broken

kc_aha.fs:
	make -f ${.CURDIR}/etc.i386/Makefile.kc \
	    IMAGE=${DESTDIR}/snapshot/kc_aha.fs \
	    KERNEL=${DESTDIR}/snapshot/netbsd-aha || \
	make -f ${.CURDIR}/etc.i386/Makefile.kc \
	    IMAGE=${DESTDIR}/snapshot/kc_aha.fs unconfig

kc_bt.fs:
	make -f ${.CURDIR}/etc.i386/Makefile.kc \
	    IMAGE=${DESTDIR}/snapshot/kc_bt.fs \
	    KERNEL=${DESTDIR}/snapshot/netbsd-bt || \
	make -f ${.CURDIR}/etc.i386/Makefile.kc \
	    IMAGE=${DESTDIR}/snapshot/kc_bt.fs unconfig

.endif	# DESTDIR check
