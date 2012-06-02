# $NetBSD: GENERIC.MP,v 1.1.2.2 2012/06/02 11:08:56 mrg Exp $
#
# Generic hp700 kernel with multiprocessor support.

include "arch/hp700/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
