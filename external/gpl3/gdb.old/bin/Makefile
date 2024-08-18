#	$NetBSD: Makefile,v 1.11 2024/08/18 04:00:42 rin Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

#SUBDIR+= sim .WAIT

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

.if ${MKGDBSERVER} != "no"
SUBDIR+= gdbserver
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= psim
.endif

.include <bsd.subdir.mk>
