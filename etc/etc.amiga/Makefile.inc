#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.4.2.1 1994/07/10 03:53:50 cgd Exp $

.ifdef DESTDIR
snap_md: netbsd-generic netbsd-a3000
	cp ${.CURDIR}/../sys/arch/amiga/compile/GENERIC/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-generic.gz
	cp ${.CURDIR}/../sys/arch/amiga/compile/A3000/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-a3000.gz
netbsd-generic:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config.new GENERIC
	cd ${.CURDIR}/../sys/arch/amiga/compile/GENERIC && \
	    make clean && make depend && make && gzip -9 netbsd

netbsd-a3000:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config.new A3000
	cd ${.CURDIR}/../sys/arch/amiga/compile/A3000 && \
	    make clean && make depend && make && gzip -9 netbsd
.endif	# DESTDIR check
