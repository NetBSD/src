#	$NetBSD: Makefile.inc,v 1.1.10.3 2017/12/03 11:39:16 jdolecek Exp $
#

.PATH:	${RUMPTOP}/librump/rumpkern/arch/x86

SRCS+=	rump_x86_cpu.c rump_x86_spl.c rump_x86_cpu_counter.c
SRCS+=	rump_x86_spinlock.c rump_x86_pmap.c
SRCS+=	rump_x86_abi.c
