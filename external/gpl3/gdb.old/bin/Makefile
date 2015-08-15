#	$NetBSD: Makefile,v 1.1.1.1 2015/08/15 09:27:49 christos Exp $

.include "../Makefile.inc"

#SUBDIR+= sim .WAIT

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

#SUBDIR+= gdbserver

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= psim
.endif

.include <bsd.subdir.mk>
