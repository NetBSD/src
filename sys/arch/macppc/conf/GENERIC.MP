# $NetBSD: GENERIC.MP,v 1.2.22.1 2008/09/18 04:33:29 wrstuden Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
