# $NetBSD: GENERIC.MP,v 1.1.6.2 2014/05/22 11:39:50 yamt Exp $
#
# Generic hppa kernel with multiprocessor support.

include "arch/hppa/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
