#	$NetBSD: SWINGER.MP,v 1.3.2.3 2004/09/21 13:16:38 skrll Exp $
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
