#	$NetBSD: Makefile,v 1.2 2011/10/09 01:59:31 christos Exp $

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
