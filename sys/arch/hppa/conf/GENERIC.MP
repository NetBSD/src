# $NetBSD: GENERIC.MP,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $
#
# Generic hppa kernel with multiprocessor support.

include "arch/hppa/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
