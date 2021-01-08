#	$NetBSD: exec.mk,v 1.3.8.1 2021/01/08 13:02:56 martin Exp $

# this makefile fragment can be included to modify the default
# ABI a program is compiled with.  this is designed to be used
# by tools that must match the kernel image like savecore(8)
# and kvm(3) using tools.

# currently this file is used by these Makefiles:
#
#   external/bsd/ipf/Makefile.inc 
#   sbin/savecore/Makefile
#   usr.bin/fstat/Makefile
#   usr.bin/netstat/Makefile
#   usr.bin/pmap/Makefile
#   usr.bin/systat/Makefile
#   usr.bin/vmstat/Makefile
#   usr.sbin/crash/Makefile
#   usr.sbin/kgmon/Makefile
#   usr.sbin/pstat/Makefile
#   usr.sbin/trpt/Makefile
#
# of these, savecore, crash and kgmon are the only ones that
# can be considered "not a bug".  all the *stat tools should
# be converted to use sysctl(3) on the running kernel, and
# anyone who needs kvm-access on crash dumps can build their
# own 64 bit version as necessary.  ipfilter doesn't use
# 64-bit alignment/size safe structures.
# 

# mips64 defaults to 32 bit userland, but with a 64 bit kernel
# most kvm-using tools are happier with 64 bit.

.if ${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb"

# XXX -pie makes n64 crash
NOPIE=1
# XXX interesting dynamic binaries crash (hello.c works.)
LDSTATIC=-static

. include <bsd.own.mk>

. if ${MKCOMPAT} != "no" && !defined(CRUNCHEDPROG)
.  include "${.PARSEDIR}/mips64/64/bsd.64.mk"
. endif # ${MKCOMPAT} != "no"

.endif # mips64
