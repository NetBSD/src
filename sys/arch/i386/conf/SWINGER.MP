#	$NetBSD: SWINGER.MP,v 1.1.2.1 2000/12/29 21:37:48 thorpej Exp $
#
#	SWINGER.MP -- Swinger, plus enable Multiprocessor support.
#

include "arch/i386/conf/SWINGER"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

#options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
#options 	DEBUG

ioapic*		at mainbus? apid ?
