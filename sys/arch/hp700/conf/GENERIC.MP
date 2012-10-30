# $NetBSD: GENERIC.MP,v 1.1.6.2 2012/10/30 17:19:35 yamt Exp $
#
# Generic hp700 kernel with multiprocessor support.

include "arch/hp700/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
