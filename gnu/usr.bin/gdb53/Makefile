#	$NetBSD: Makefile,v 1.3 2004/04/02 13:36:32 mrg Exp $

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
