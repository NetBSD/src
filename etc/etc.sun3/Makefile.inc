#	$NetBSD: Makefile.inc,v 1.2 1997/09/20 03:05:39 mikel Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd-gen netbsd-dl

netbsd-gen:
	cd ${.CURDIR}/../sys/arch/sun3/compile/GENERIC && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3/compile/GENERIC/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-gen.gz

netbsd-dl:
	cd ${.CURDIR}/../sys/arch/sun3/compile/DISKLESS && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3/compile/DISKLESS/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-dl.gz

.endif	# DESTDIR check
