# $NetBSD: GENERIC.MP,v 1.4.40.1 2008/03/24 07:15:04 keiichi Exp $
#
# GENERIC sparc64 kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident 	"GENERIC.MP-$Revision: 1.4.40.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
