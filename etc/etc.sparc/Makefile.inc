#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.4.2.2 1994/08/12 06:34:39 mycroft Exp $

.ifdef DESTDIR
snap_md: netbsd netbsd.scsi3 inst
	cp ${.CURDIR}/../sys/arch/sparc/compile/SPARC/netbsd \
	    ${DESTDIR}/snapshot/netbsd
	cp ${.CURDIR}/../sys/arch/sparc/compile/SPARC_SCSI3/netbsd \
	    ${DESTDIR}/snapshot/netbsd.scsi3

netbsd:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config.new SPARC
	cd ${.CURDIR}/../sys/arch/sparc/compile/SPARC && \
	    make clean && make depend && make

netbsd.scsi3:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config.new SPARC_SCSI3
	cd ${.CURDIR}/../sys/arch/sparc/compile/SPARC_SCSI3 && \
	    make clean && make depend && make

inst:
	cp ${.CURDIR}/etc.sparc/README ${DESTDIR}/snapshot/README
	cp ${.CURDIR}/etc.sparc/install.sh ${DESTDIR}/snapshot/install.sh

.endif	# DESTDIR check
