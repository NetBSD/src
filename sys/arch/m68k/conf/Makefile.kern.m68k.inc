#	$NetBSD: Makefile.kern.m68k.inc,v 1.4 2026/11/02 13:43:39 thorpej Exp $

# Special make rule to build fpu files in the kernel and avoid passing
# -msoft-float.  Use this with config(1) file, eg:
#   file arch/m68k/m68k/fpu.c compile-with "${M68K_KERN_FPU}"

M68K_KERN_FPU= \
	${CC} ${COPTS.${<:T}} ${CFLAGS:N-msoft-float} ${CPPFLAGS} -c $< -o $@

# Kernel debugging and profiling require the frame pointer.
HAVE_DDB!=	grep '\#define[ 	]*DDB' opt_ddb.h 2>/dev/null || echo no
HAVE_KGDB!=	grep '\#define[ 	]*KGDB' opt_kgdb.h 2>/dev/null || echo no

.if defined(PROF) || ${HAVE_DDB} != "no" || ${HAVE_KGDB} != "no"
CFLAGS+=	-fno-omit-frame-pointer
.endif

#
# Turn off all unwind tables; they're not terribly useful in our
# kernel environment and add a lot of needless bulk.
#
CFLAGS+=	-fno-unwind-tables
