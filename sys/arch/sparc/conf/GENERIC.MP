# $NetBSD: GENERIC.MP,v 1.4 2005/12/11 12:19:05 christos Exp $
#
# Generic Sparc kernel configuration with multiprocessor support.

include	"arch/sparc/conf/GENERIC"

options 	MULTIPROCESSOR		# include multiprocessor support

## disable inlining __cpu_simple_lock()
## the locore version attempts to detect stuck locks
## not inlining saves about 60K on the kernel's text
options 	__CPU_SIMPLE_LOCK_NOINLINE

cpu* 		at mainbus0		# declare additional CPUs
cpu*		at cpuunit?
