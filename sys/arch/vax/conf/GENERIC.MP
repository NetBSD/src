# $NetBSD: GENERIC.MP,v 1.1 2000/03/19 14:56:52 ragge Exp $
#
# Generic VAX kernel with multiprocessor support.

include "arch/vax/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support
