#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.1 1994/03/05 08:06:47 cgd Exp $

.ifdef DESTDIR
snap_md: netbsd-aha netbsd-ahbbt
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICAHA/netbsd \
	    ${DESTDIR}/snapshot/netbsd-aha
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICAHBBT/netbsd \
	    ${DESTDIR}/snapshot/netbsd-ahbbt
netbsd-aha:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICAHA
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICAHA && \
	    make clean && make depend && make

netbsd-ahbbt:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICAHBBT
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICAHBBT && \
	    make clean && make depend && make
.endif	# DESTDIR check
