# $NetBSD: GENERIC.MP,v 1.4.44.2 2008/06/29 09:33:00 mjf Exp $
#
# GENERIC kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident		"GENERIC.MP-$Revision: 1.4.44.2 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
