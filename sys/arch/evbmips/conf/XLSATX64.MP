# $NetBSD: XLSATX64.MP,v 1.1.10.1 2011/03/05 15:09:37 bouyer Exp $
#
include "arch/evbmips/conf/XLSATX64"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no cpucore*
no cpu*
cpucore0 at cpunode0 core 0
cpu0 at cpucore0 thread 0
cpucore1 at cpunode0 core 1
cpu4 at cpucore1 thread 0
#cpucore2 at cpunode0 core 2
#cpu8 at cpucore2 thread 0
#cpucore3 at cpunode0 core 3
#cpu12 at cpucore3 thread 0
