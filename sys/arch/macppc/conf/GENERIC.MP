# $NetBSD: GENERIC.MP,v 1.1.70.1 2007/10/23 20:13:14 ad Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
