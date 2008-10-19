# $NetBSD: GENERIC.MP,v 1.2.26.1 2008/10/19 22:15:51 haad Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
