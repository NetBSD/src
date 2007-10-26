# $NetBSD: GENERIC.MP,v 1.1.88.1 2007/10/26 15:42:43 joerg Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
