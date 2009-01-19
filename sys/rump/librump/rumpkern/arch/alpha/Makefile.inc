#	$NetBSD: Makefile.inc,v 1.1.4.2 2009/01/19 13:20:26 skrll Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c
