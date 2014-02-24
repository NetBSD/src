# $NetBSD: GENERIC.MP,v 1.1 2014/02/24 07:23:42 skrll Exp $
#
# Generic hppa kernel with multiprocessor support.

include "arch/hppa/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
