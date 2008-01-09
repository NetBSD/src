#	$NetBSD: INSTALL.MP,v 1.9.28.1 2008/01/09 01:46:28 matt Exp $
#
#	INSTALL.MP -- Install, plus enable Multiprocessor support.
#	Mostly to let people test-boot MP systems.

include "arch/i386/conf/INSTALL"

options 	MULTIPROCESSOR

options 	APM_NO_IDLE
options 	DIAGNOSTIC	# STRONGLY RECOMMENDED
options 	LOCKDEBUG
options 	MPDEBUG
options 	MPVERBOSE
options 	DEBUG
options 	MPBIOS		# configure CPUs and APICs using MPBIOS

no makeoptions 	CPUFLAGS
makeoptions 	CPUFLAGS="-march=pentium -mtune=i486"
makeoptions 	COPTS="-Os"

ioapic*		at mainbus? apid ?
