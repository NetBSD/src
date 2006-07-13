# $NetBSD: GENERIC.MP,v 1.2.16.1 2006/07/13 17:49:04 gdamore Exp $
#
# GENERIC Sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.2.16.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
