#	$NetBSD: INSTALL.MP,v 1.9.26.1 2007/11/21 21:53:11 joerg Exp $
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

no makeoptions	CPUFLAGS
makeoptions	CPUFLAGS="-march=pentium -mtune=i486"
makeoptions	COPTS="-Os"

ioapic*		at mainbus? apid ?
