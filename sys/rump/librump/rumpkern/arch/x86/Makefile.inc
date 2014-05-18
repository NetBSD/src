#	$NetBSD: Makefile.inc,v 1.1.4.2 2014/05/18 17:46:19 rmind Exp $
#

.PATH:	${RUMPTOP}/librump/rumpkern/arch/x86

SRCS+=	rump_x86_cpu.c rump_x86_spl.c rump_x86_cpu_counter.c
SRCS+=	rump_x86_spinlock.c rump_x86_pmap.c
