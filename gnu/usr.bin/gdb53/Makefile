#	$NetBSD: Makefile,v 1.2 2003/09/26 17:51:18 christos Exp $

.include "Makefile.inc"

SUBDIR=		bfd libiberty opcodes
.if ${USE_LIBEDIT} == no || ${USE_TUI} == yes
SUBDIR+=	readline
.endif
SUBDIR+= gdb # gdbreplay gdbserver

.include <bsd.subdir.mk>
