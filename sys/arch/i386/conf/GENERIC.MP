#	$NetBSD: GENERIC.MP,v 1.1.2.2 2000/09/23 17:19:05 sommerfeld Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options		DEBUG

ioapic*		at mainbus? apid ?
