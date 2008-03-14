# $NetBSD: GENERIC.MP,v 1.5 2008/03/14 16:43:16 nakayama Exp $
#
# GENERIC sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.5 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
