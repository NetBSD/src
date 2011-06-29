# $NetBSD: P2020DS.MP,v 1.1 2011/06/29 06:23:36 matt Exp $
#
#	P2020DS kernel, plus multiprocessor support.

include	"arch/evbppc/conf/P2020DS"

options 	MULTIPROCESSOR
# this option may cause trouble under very high interrupt load
#options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
