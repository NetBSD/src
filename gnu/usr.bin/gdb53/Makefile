#	$NetBSD: Makefile,v 1.2.2.1 2004/04/06 04:33:45 jmc Exp $

.include "Makefile.inc"

SUBDIR=		bfd libiberty opcodes
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	readline
.endif
.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpcle"
SUBDIR+= sim .WAIT
.endif
SUBDIR+= gdb # gdbreplay gdbserver

.include <bsd.subdir.mk>
