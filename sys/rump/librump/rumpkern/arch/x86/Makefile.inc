#	$NetBSD: Makefile.inc,v 1.1.12.1 2015/06/06 14:40:29 skrll Exp $
#

.PATH:	${RUMPTOP}/librump/rumpkern/arch/x86

SRCS+=	rump_x86_cpu.c rump_x86_spl.c rump_x86_cpu_counter.c
SRCS+=	rump_x86_spinlock.c rump_x86_pmap.c
SRCS+=	rump_x86_abi.c
