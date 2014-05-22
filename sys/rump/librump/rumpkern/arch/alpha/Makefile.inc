#	$NetBSD: Makefile.inc,v 1.2.12.1 2014/05/22 11:41:15 yamt Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_pmap.c
