# $NetBSD: GENERIC.MP,v 1.1 1998/09/27 15:10:25 pk Exp $
#
# Generic Sparc kernel configuration with multiprocessor support.

include	"arch/sparc/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

cpu* 		at mainbus0		# declare additional CPUs
