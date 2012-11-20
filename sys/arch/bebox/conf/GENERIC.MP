# $NetBSD: GENERIC.MP,v 1.1.4.2 2012/11/20 03:01:10 tls Exp $

include 	"arch/bebox/conf/GENERIC"

# SMP options
options 	MULTIPROCESSOR
options 	CACHE_PROTO_MEI			# require PPC 603e
