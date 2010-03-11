#	$NetBSD: Makefile.inc,v 1.1.8.3 2010/03/11 15:04:38 yamt Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c rumpcpu_generic.c
