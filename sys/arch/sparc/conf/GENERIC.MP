# $NetBSD: GENERIC.MP,v 1.2 2004/04/03 17:43:50 chs Exp $
#
# Generic Sparc kernel configuration with multiprocessor support.

include	"arch/sparc/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

cpu* 		at mainbus0		# declare additional CPUs
cpu*		at cpuunit?
