#	$NetBSD: GENERIC.MP,v 1.7.8.1 2006/08/11 15:41:53 yamt Exp $
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

ioapic*		at mainbus? apid ?
