# $NetBSD: GENERIC.MP,v 1.1 2012/10/20 14:53:37 kiyohara Exp $

include 	"arch/bebox/conf/GENERIC"

# SMP options
options 	MULTIPROCESSOR
options 	CACHE_PROTO_MEI			# require PPC 603e
