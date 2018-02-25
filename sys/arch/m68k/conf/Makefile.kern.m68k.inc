#	$NetBSD: Makefile.kern.m68k.inc,v 1.1 2018/02/25 00:27:16 mrg Exp $

# Special make rule to build fpu files in the kernel and avoid passing
# -msoft-float.  Use this with config(1) file, eg:
#   file arch/m68k/m68k/fpu.c compile-with "${M68K_KERN_FPU}"

M68K_KERN_FPU= \
	${CC} ${COPTS.${<:T}} ${CFLAGS:N-msoft-float} ${CPPFLAGS} -c $< -o $@
