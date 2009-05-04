# $NetBSD: GENERIC32.MP,v 1.1.20.2 2009/05/04 08:11:57 yamt Exp $
#
# GENERIC kernel configuration for 32-bit kernel with multiprocessor support.
#

include 	"arch/sparc64/conf/GENERIC32"

#ident		"GENERIC32.MP-$Revision: 1.1.20.2 $"

options 	MULTIPROCESSOR		# include multiprocessor support
cpu* 		at mainbus0		# declare additional CPUs
