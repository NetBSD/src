#	$NetBSD: Makefile,v 1.1 2011/09/26 03:02:44 christos Exp $

.include "../Makefile.inc"

#SUBDIR+= sim .WAIT

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

#SUBDIR+= gdbserver

.include <bsd.subdir.mk>
