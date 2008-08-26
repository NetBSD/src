# $NetBSD: GENERIC.MP,v 1.3 2008/08/26 16:55:48 macallan Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
