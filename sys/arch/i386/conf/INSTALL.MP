#	$NetBSD: INSTALL.MP,v 1.8 2006/11/28 16:15:37 christos Exp $
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

no options	I486_CPU
no makeoptions	CPUFLAGS
makeoptions	CPUFLAGS="-Os -march=pentium -mtune=i486"

ioapic*		at mainbus? apid ?
