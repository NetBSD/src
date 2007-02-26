# $NetBSD: GENERIC.MP,v 1.1.18.2 2007/02/26 09:08:22 yamt Exp $
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

#ident 	"GENERIC.MP-$Revision: 1.1.18.2 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
