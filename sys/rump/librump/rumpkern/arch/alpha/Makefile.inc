#	$NetBSD: Makefile.inc,v 1.1 2009/01/06 17:04:01 pooka Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c
