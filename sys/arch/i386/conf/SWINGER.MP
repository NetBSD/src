#	$NetBSD: SWINGER.MP,v 1.5 2005/12/11 12:17:41 christos Exp $
#
#	SWINGER.MP -- Swinger, plus enable Multiprocessor support.
#

include "arch/i386/conf/SWINGER"

options 	MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

#options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options 	MPBIOS
#options 	DEBUG

ioapic*		at mainbus? apid ?
