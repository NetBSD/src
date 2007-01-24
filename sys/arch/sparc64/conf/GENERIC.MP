# $NetBSD: GENERIC.MP,v 1.4 2007/01/24 05:53:56 mrg Exp $
#
# GENERIC Sparc64 kernel configuration with multiprocessor support.
#
# XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
#    
#     NOTE:  THIS KERNEL DOES NOT YET WORK.  IT SHOULD COMPILE.
#
# XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.4 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
