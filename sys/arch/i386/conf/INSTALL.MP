#	$NetBSD: INSTALL.MP,v 1.2.8.3 2004/09/21 13:16:36 skrll Exp $
#
#	INSTALL.MP -- Install, plus enable Multiprocessor support.
#	Mostly to let people test-boot MP systems.

include "arch/i386/conf/INSTALL"

options 	MULTIPROCESSOR
options 	COM_MPLOCK	# com MP locking; REQUIRED on MP i386

options 	APM_NO_IDLE
options 	DIAGNOSTIC	# STRONGLY RECOMMENDED
options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options 	DEBUG
options 	MPBIOS		# configure CPUs and APICs using MPBIOS
#options 	MPACPI		# configure CPUs and APICs using ACPI
				# (acpi at mainbus must also be enabled)

ioapic*		at mainbus? apid ?
