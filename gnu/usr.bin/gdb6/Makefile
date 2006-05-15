#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

.include "Makefile.inc"

SUBDIR=		bfd libiberty opcodes libgdb
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	readline
.endif

SUBDIR+= .WAIT

.if ${MACHINE_ARCH} == "powerpc" || ${MACHINE_ARCH} == "powerpcle"
SUBDIR+= sim
.endif

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

#SUBDIR+= gdbserver

.include <bsd.subdir.mk>
