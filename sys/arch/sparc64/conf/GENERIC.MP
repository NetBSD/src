# $NetBSD: GENERIC.MP,v 1.5.8.1 2008/07/03 18:37:55 simonb Exp $
#
# GENERIC kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident		"GENERIC.MP-$Revision: 1.5.8.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
