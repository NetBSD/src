#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.3 1994/04/20 12:44:53 deraadt Exp $

.ifdef DESTDIR
snap_md: netbsd-sd0 netbsd-sd1 inst
	cp ${.CURDIR}/../sys/arch/sparc/compile/TDR/netbsd \
	    ${DESTDIR}/snapshot/netbsd-sd1
	cp ${.CURDIR}/../sys/arch/sparc/compile/TDR2/netbsd \
	    ${DESTDIR}/snapshot/netbsd-sd0

netbsd-sd1:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config.new TDR
	cd ${.CURDIR}/../sys/arch/sparc/compile/TDR && \
	    make clean && make depend && make

netbsd-sd0:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config.new TDR2
	cd ${.CURDIR}/../sys/arch/sparc/compile/TDR2 && \
	    make clean && make depend && make

inst:
	cp ${.CURDIR}/etc.sparc/README ${DESTDIR}/snapshot/README
	cp ${.CURDIR}/etc.sparc/install.sh ${DESTDIR}/snapshot/install.sh
	cd ${DESTDIR} && find dev -print | cpio -o | gzip -9 > snapshot/dev.cpio.gz

.endif	# DESTDIR check
