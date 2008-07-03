# $NetBSD: GENERIC32.MP,v 1.1.4.2 2008/07/03 18:37:55 simonb Exp $
#
# GENERIC kernel configuration for 32-bit kernel with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC32"

#ident		"GENERIC32.MP-$Revision: 1.1.4.2 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
