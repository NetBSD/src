# $NetBSD: GENERIC.MP,v 1.3 2006/06/30 10:27:48 tsutsui Exp $
#
# GENERIC Sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.3 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
