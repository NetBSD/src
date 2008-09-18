# $NetBSD: GENERIC.MP,v 1.5.6.1 2008/09/18 04:33:34 wrstuden Exp $
#
# GENERIC kernel configuration with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC"

#ident	 	"GENERIC.MP-$Revision: 1.5.6.1 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
