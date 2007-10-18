# $NetBSD: GENERIC.MP,v 1.1.92.1 2007/10/18 08:32:04 yamt Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
