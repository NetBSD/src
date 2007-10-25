# $NetBSD: GENERIC.MP,v 1.1.94.1 2007/10/25 22:35:57 bouyer Exp $
#
#	GENERIC kernel, plus multiprocessor support.

include	"arch/macppc/conf/GENERIC"

options 	MULTIPROCESSOR
options		IPI_HAMMERHEAD
options		IPI_OPENPIC
