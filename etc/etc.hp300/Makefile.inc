#	$NetBSD: Makefile.inc,v 1.3 1997/03/06 23:41:26 mikel Exp $
#
#	etc.hp300/Makefile.inc -- hp300-specific etc Makefile targets

.ifdef DESTDIR
snap_md: netbsd-gen.gz netbsd-dl.gz

netbsd-gen:
	cd ${.CURDIR}/../sys/arch/hp300/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/hp300/compile/GENERIC && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/hp300/compile/GENERIC/netbsd \
	    ${DESTDIR}/snapshot/netbsd-gen

netbsd-dl:
	cd ${.CURDIR}/../sys/arch/hp300/conf && config DISKLESS
	cd ${.CURDIR}/../sys/arch/hp300/compile/DISKLESS && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/hp300/compile/DISKLESS/netbsd \
	    ${DESTDIR}/snapshot/netbsd-dl

netbsd-gen.gz: netbsd-gen
	gzip -f9 ${DESTDIR}/snapshot/netbsd-gen

netbsd-dl.gz: netbsd-dl
	gzip -f9 ${DESTDIR}/snapshot/netbsd-dl

.endif	# DESTDIR check
