#	$NetBSD: INSTALL.MP,v 1.1.2.2 2000/09/23 17:19:05 sommerfeld Exp $
#
#	INSTALL.MP -- Install, plus enable Multiprocessor support.
#	Mostly to let people test-boot MP systems.
#

include "arch/i386/conf/INSTALL"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options		DEBUG

ioapic*		at mainbus? apid ?
