# $NetBSD: GENERIC.MP,v 1.2 1999/02/22 19:14:46 thorpej Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

makeoptions 	DEBUG="-g"		# ...and full symbols
