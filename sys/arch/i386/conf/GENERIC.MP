#	$NetBSD: GENERIC.MP,v 1.4 2003/03/05 10:28:00 grant Exp $
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
				# (acpi at mainbus must also be enabled)

ioapic*		at mainbus? apid ?
