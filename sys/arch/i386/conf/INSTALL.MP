#	$NetBSD: INSTALL.MP,v 1.2 2002/10/01 12:56:43 fvdl Exp $
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
