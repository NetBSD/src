#	$NetBSD: INSTALL.MP,v 1.1.2.1 2000/02/21 19:34:05 sommerfeld Exp $
#
#	INSTALL.MP -- Install, plus enable Multiprocessor support.
#	Mostly to let people test-boot MP systems.
#

include "arch/i386/conf/INSTALL"

options		MULTIPROCESSOR
options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options		DEBUG

ioapic*		at mainbus? apid ?
