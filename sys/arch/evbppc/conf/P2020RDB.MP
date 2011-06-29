# $NetBSD: P2020RDB.MP,v 1.1 2011/06/29 06:23:36 matt Exp $
#
#	P2020RDB kernel, plus multiprocessor support.

include	"arch/evbppc/conf/P2020RDB"

options 	MULTIPROCESSOR
# this option may cause trouble under very high interrupt load
#options 	OPENPIC_DISTRIBUTE	# let all CPUs serve interrupts
