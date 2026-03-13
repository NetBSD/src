#	$NetBSD: Makefile,v 1.13 2026/03/13 22:08:57 christos Exp $

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
