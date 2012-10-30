# $NetBSD: TWRP1025.MP,v 1.1.4.2 2012/10/30 17:19:30 yamt Exp $
#
#	TWRP1025 kernel, plus multiprocessor support.

include	"arch/evbppc/conf/TWRP1025"

options 	MULTIPROCESSOR
# this option may cause trouble under very high interrupt load
#options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
