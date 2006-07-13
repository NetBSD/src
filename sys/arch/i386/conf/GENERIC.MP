#	$NetBSD: GENERIC.MP,v 1.7.16.1 2006/07/13 17:48:52 gdamore Exp $
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
