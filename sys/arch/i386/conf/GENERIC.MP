#	$NetBSD: GENERIC.MP,v 1.1.2.1 2000/02/20 18:38:49 sommerfeld Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options		MULTIPROCESSOR
options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options		DEBUG

ioapic*		at mainbus? apid ?
