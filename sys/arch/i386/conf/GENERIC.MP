#	$NetBSD: GENERIC.MP,v 1.3 2003/01/07 18:55:36 fvdl Exp $
#
#	GENERIC.MP -- Generic, plus enable Multiprocessor support.
#

include "arch/i386/conf/GENERIC"

options		MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options 	APM_NO_IDLE
#options	DIAGNOSTIC	# STRONGLY RECOMMENDED
#options 	LOCKDEBUG
#options 	MPDEBUG
#options 	MPVERBOSE
#options	DEBUG
options 	MPBIOS		# configure CPUs and APICs using MPBIOS
#options 	MPACPI		# configure CPUs and APICs using ACPI
				# (acpi at mainbus must also be enable)

ioapic*		at mainbus? apid ?
