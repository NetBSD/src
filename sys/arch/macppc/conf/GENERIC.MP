# $NetBSD: GENERIC.MP,v 1.1.78.1 2007/10/10 18:41:31 garbled Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
