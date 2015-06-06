#	$NetBSD: Makefile.inc,v 1.3.6.1 2015/06/06 14:40:29 skrll Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_abi.c rump_generic_cpu.c rump_generic_pmap.c
