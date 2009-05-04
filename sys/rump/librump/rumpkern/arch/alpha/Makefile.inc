#	$NetBSD: Makefile.inc,v 1.1.8.2 2009/05/04 08:14:30 yamt Exp $
#

# some stubs
SRCS+=	rumpcrud.c

.PATH:  ${RUMPTOP}/../arch/alpha/alpha
SRCS+=  kobj_machdep.c
