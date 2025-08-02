#	$NetBSD: Makefile,v 1.5.4.1 2025/08/02 05:26:16 perseant Exp $

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
