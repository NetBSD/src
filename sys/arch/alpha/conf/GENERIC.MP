# $NetBSD: GENERIC.MP,v 1.2.22.1 2002/01/10 19:37:02 thorpej Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

makeoptions	DEBUG="-g"		# ...and full symbols
