#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.4.2.1 1994/07/28 09:00:34 cgd Exp $

.ifdef DESTDIR
snap_md: netbsd inst
	cp ${.CURDIR}/../sys/arch/sparc/compile/SPARC/netbsd \
	    ${DESTDIR}/snapshot/netbsd

netbsd:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config.new SPARC
	cd ${.CURDIR}/../sys/arch/sparc/compile/SPARC && \
	    make clean && make depend && make

inst:
	cp ${.CURDIR}/etc.sparc/README ${DESTDIR}/snapshot/README
	cp ${.CURDIR}/etc.sparc/install.sh ${DESTDIR}/snapshot/install.sh

.endif	# DESTDIR check
