# $NetBSD: GENERIC.MP,v 1.2.20.1 2002/01/08 00:22:53 nathanw Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

makeoptions	DEBUG="-g"		# ...and full symbols
