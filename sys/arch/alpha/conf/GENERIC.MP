# $NetBSD: GENERIC.MP,v 1.1 1998/09/25 00:46:09 thorpej Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

options 	DDB			# not stable yet; include DDB
makeoptions 	DEBUG="-g"		# ...and full symbols
