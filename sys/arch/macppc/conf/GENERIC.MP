# $NetBSD: GENERIC.MP,v 1.2.16.1 2008/09/28 10:40:03 mjf Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
