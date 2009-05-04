# $NetBSD: GENERIC.MP,v 1.5.4.1 2009/05/04 08:11:57 yamt Exp $
#
# GENERIC kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident		"GENERIC.MP-$Revision: 1.5.4.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
