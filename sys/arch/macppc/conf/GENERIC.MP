# $NetBSD: GENERIC.MP,v 1.1.28.1 2007/10/27 11:27:00 yamt Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
