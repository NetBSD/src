#	$NetBSD: Makefile.inc,v 1.2 2009/10/15 23:42:40 pooka Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c rumpcpu_generic.c
