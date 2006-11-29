#	$NetBSD: Makefile,v 1.3 2006/11/29 15:18:10 he Exp $

.include "Makefile.inc"

SUBDIR=		bfd libiberty opcodes libgdb
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	readline
.endif

SUBDIR+= sim .WAIT

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

#SUBDIR+= gdbserver

.include <bsd.subdir.mk>
