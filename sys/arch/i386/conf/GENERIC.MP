#	$NetBSD: GENERIC.MP,v 1.1.2.4 2001/04/30 16:23:08 sommerfeld Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options		DIAGNOSTIC	# STRONGLY RECOMMENDED

#options 	LOCKDEBUG
#options 	MPDEBUG
#options 	MPVERBOSE
#options	DEBUG

ioapic*		at mainbus? apid ?
