# $NetBSD: GENERIC.MP,v 1.1.6.2 2000/11/20 20:32:43 bouyer Exp $
#
# Generic VAX kernel with multiprocessor support.

include "arch/vax/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
