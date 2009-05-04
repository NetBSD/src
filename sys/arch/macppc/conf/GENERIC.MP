# $NetBSD: GENERIC.MP,v 1.2.20.1 2009/05/04 08:11:28 yamt Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
