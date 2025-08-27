#	$NetBSD: Makefile,v 1.12 2025/08/27 15:20:56 christos Exp $

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

#.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
#    ${MACHINE_ARCH} == powerpc64
#SUBDIR+= psim
#.endif

.include <bsd.subdir.mk>
