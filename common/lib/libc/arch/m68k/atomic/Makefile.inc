#	$NetBSD: Makefile.inc,v 1.4 2008/02/11 14:21:12 ad Exp $

#
# Note: The atomic operations here in these assembly files are atomic
# only with respect to regular memory on uniprocessor systems.  Since
# we don't support any MP m68k systems, this is just fine.  If we ever
# do, then these routines will probably need to be replaced with CAS-
# based routines (CAS generates an atomic bus cycle, whereas these
# others are merely single-instruction).
#

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic_add.S atomic_and.S atomic_cas.S atomic_dec.S \
	atomic_inc.S atomic_or.S atomic_swap.S membar_ops_nop.c

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
