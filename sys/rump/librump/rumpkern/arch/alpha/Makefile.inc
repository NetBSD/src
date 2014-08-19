#	$NetBSD: Makefile.inc,v 1.2.22.1 2014/08/20 00:04:41 tls Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_pmap.c
