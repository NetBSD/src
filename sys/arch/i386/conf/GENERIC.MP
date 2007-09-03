#	$NetBSD: GENERIC.MP,v 1.5.12.3 2007/09/03 14:26:30 yamt Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options 	MULTIPROCESSOR

options 	APM_NO_IDLE
#options 	DIAGNOSTIC	# STRONGLY RECOMMENDED
#options 	LOCKDEBUG
#options 	MPDEBUG
#options 	MPVERBOSE
#options 	DEBUG
options 	MPBIOS		# configure CPUs and APICs using MPBIOS
