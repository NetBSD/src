#	$NetBSD: Makefile,v 1.2 2006/05/29 19:15:30 nathanw Exp $

.include "Makefile.inc"

SUBDIR=		bfd libiberty opcodes libgdb
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	readline
.endif

SUBDIR+= .WAIT

SUBDIR+= sim

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

#SUBDIR+= gdbserver

.include <bsd.subdir.mk>
