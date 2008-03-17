# $NetBSD: GENERIC.MP,v 1.1.18.3 2008/03/17 09:14:24 yamt Exp $
#
# GENERIC sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.1.18.3 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
