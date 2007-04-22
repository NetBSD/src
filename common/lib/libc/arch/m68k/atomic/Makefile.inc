#	$NetBSD: Makefile.inc,v 1.1.2.1 2007/04/22 04:14:18 thorpej Exp $

#
# Note: The atomic operations here in these assembly files are atomic
# only with respect to regular memory on uniprocessor systems.  Since
# we don't support any MP m68k systems, this is just fine.  If we ever
# do, then these routines will probably need to be replaced with CAS-
# based routines (CAS generates an atomic bus cycle, whereas these
# others are merely single-instruction).
#

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic_add.S

SRCS+=	atomic_and.S

SRCS+=	atomic_cas.S

SRCS+=	atomic_dec.S

SRCS+=	atomic_inc.S

SRCS+=	atomic_or.S

SRCS+=	atomic_swap.S

SRCS+=	membar_ops_nop.c

.endif
