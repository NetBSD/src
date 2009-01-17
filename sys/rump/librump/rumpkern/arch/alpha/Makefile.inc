#	$NetBSD: Makefile.inc,v 1.1.2.2 2009/01/17 13:29:36 mjf Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c
