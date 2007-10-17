# $NetBSD: GENERIC.MP,v 1.2 2007/10/17 19:55:16 garbled Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
