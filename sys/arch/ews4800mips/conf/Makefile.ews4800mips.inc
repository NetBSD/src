# $NetBSD: Makefile.ews4800mips.inc,v 1.1 2017/12/24 16:15:53 christos Exp $

.if ${KERNEL_BUILD} == "RAMDISK"
COMPAT_AS=library
.endif
