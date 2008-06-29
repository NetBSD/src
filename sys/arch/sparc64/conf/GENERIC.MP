# $NetBSD: GENERIC.MP,v 1.6 2008/06/29 08:05:43 nakayama Exp $
#
# GENERIC kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident		"GENERIC.MP-$Revision: 1.6 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
