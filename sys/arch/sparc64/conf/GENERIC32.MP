# $NetBSD: GENERIC32.MP,v 1.1.2.2 2008/06/29 09:33:00 mjf Exp $
#
# GENERIC kernel configuration for 32-bit kernel with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC32"

#ident		"GENERIC32.MP-$Revision: 1.1.2.2 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
