# $NetBSD: GENERIC.MP,v 1.1.90.1 2007/11/06 23:18:30 matt Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
