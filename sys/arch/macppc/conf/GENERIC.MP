# $NetBSD: GENERIC.MP,v 1.3.14.1 2010/08/17 06:44:50 uebayasi Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
# this option may cause trouble under very high interrupt load
#options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
