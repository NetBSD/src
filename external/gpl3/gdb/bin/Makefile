#	$NetBSD: Makefile,v 1.3 2020/12/05 21:27:06 christos Exp $

.include "../Makefile.inc"

#SUBDIR+= sim .WAIT

SUBDIR+=  gdb
.if ${USE_TUI} == yes
SUBDIR+= gdbtui
.endif

.if ${MACHINE} == "amd64"
SUBDIR+= gdbserver
.endif

.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle || \
    ${MACHINE_ARCH} == powerpc64
SUBDIR+= psim
.endif

.include <bsd.subdir.mk>
