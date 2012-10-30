# $NetBSD: GENERIC.MP,v 1.1.2.2 2012/10/30 17:19:14 yamt Exp $

include 	"arch/bebox/conf/GENERIC"

# SMP options
options 	MULTIPROCESSOR
options 	CACHE_PROTO_MEI			# require PPC 603e
