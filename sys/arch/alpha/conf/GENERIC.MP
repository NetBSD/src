# $NetBSD: GENERIC.MP,v 1.3 2001/11/20 12:56:19 lukem Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

makeoptions	DEBUG="-g"		# ...and full symbols
