#	$NetBSD: SWINGER.MP,v 1.2.2.2 2002/10/18 02:37:34 nathanw Exp $
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
