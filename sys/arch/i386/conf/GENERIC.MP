#	$NetBSD: GENERIC.MP,v 1.2.2.2 2002/10/18 02:37:28 nathanw Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options 	APM_NO_IDLE
#options	DIAGNOSTIC	# STRONGLY RECOMMENDED
#options 	LOCKDEBUG
#options 	MPDEBUG
#options 	MPVERBOSE
#options	DEBUG

ioapic*		at mainbus? apid ?
