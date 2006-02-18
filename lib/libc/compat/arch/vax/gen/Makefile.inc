#	$NetBSD: Makefile.inc,v 1.1 2006/02/18 22:54:51 matt Exp $

# objects built from assembler sources (need lint stubs)
.if ${OBJECT_FMT} != "ELF"
SRCS+=	compat_setjmp.S compat_sigsetjmp.S
.endif
