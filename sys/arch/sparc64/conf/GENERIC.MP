# GENERIC.MP,v 1.4 2007/01/24 05:53:56 mrg Exp
#
# GENERIC sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-1.4"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
