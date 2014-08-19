#	$NetBSD: Makefile.inc,v 1.1.10.2 2014/08/20 00:04:41 tls Exp $
#

.PATH:	${RUMPTOP}/librump/rumpkern/arch/x86

SRCS+=	rump_x86_cpu.c rump_x86_spl.c rump_x86_cpu_counter.c
SRCS+=	rump_x86_spinlock.c rump_x86_pmap.c
