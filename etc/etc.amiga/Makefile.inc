#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#
#	$NetBSD: Makefile.inc,v 1.8 1996/10/09 00:40:33 jtc Exp $

.ifdef DESTDIR
snap_md: netbsd-generic
	cp ${.CURDIR}/../sys/arch/amiga/compile/GENERIC/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-generic.gz
netbsd-generic:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/amiga/compile/GENERIC && \
	    make clean && make depend && make && gzip -c -9 netbsd >netbsd.gz
.endif	# DESTDIR check
