#	$NetBSD: Makefile.inc,v 1.1.1.1 1997/12/24 02:33:05 jeremy Exp $
#
#	etc.sun3x/Makefile.inc -- sun3x-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd-gen netbsd-dl

netbsd-gen:
	cd ${.CURDIR}/../sys/arch/sun3x/compile/GENERIC && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3x/compile/GENERIC/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-gen.gz

netbsd-dl:
	cd ${.CURDIR}/../sys/arch/sun3x/compile/DISKLESS && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3x/compile/DISKLESS/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-dl.gz

.endif	# DESTDIR check
