#	$NetBSD: INSTALL.MP,v 1.6.4.1 2006/11/18 21:29:18 ad Exp $
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
no makeoptions	COPTS
makeoptions	COPTS="-Os -march=pentium -mtune=i486"

ioapic*		at mainbus? apid ?
