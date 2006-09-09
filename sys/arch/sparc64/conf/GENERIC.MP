# $NetBSD: GENERIC.MP,v 1.2.4.1 2006/09/09 02:43:35 rpaulo Exp $
#
# GENERIC Sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.2.4.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
