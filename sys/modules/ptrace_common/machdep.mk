# $NetBSD: machdep.mk,v 1.1 2020/10/19 19:33:02 christos Exp $

# Paths to find machine dependent sources

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/x86/x86
.endif
.if ${MACHINE_ARCH} == "powerpc64"
.PATH:	${S}/arch/powerpc/powerpc
.endif
