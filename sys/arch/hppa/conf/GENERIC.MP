# $NetBSD: GENERIC.MP,v 1.1.4.2 2014/05/18 17:45:10 rmind Exp $
#
# Generic hppa kernel with multiprocessor support.

include "arch/hppa/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
