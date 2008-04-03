# $NetBSD: GENERIC.MP,v 1.4.44.1 2008/04/03 12:42:25 mjf Exp $
#
# GENERIC sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.4.44.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
