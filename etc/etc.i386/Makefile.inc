#	$NetBSD: Makefile.inc,v 1.9 1997/03/04 06:30:01 mikel Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd

netbsd:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERIC && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERIC/netbsd \
	    ${DESTDIR}/snapshot/netbsd

.endif	# DESTDIR check
