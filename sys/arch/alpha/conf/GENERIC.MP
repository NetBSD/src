# $NetBSD: GENERIC.MP,v 1.4 2020/09/05 01:02:02 thorpej Exp $
#
# Generic Alpha kernel with multiprocessor support.

include	"arch/alpha/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
