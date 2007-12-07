#	$NetBSD: Makefile.arch.inc,v 1.1.18.1 2007/12/07 17:27:03 yamt Exp $

.if defined(MACHINE) && ${MACHINE} != "xen"
XEN_BUILD?=	${MACHINE}
.else
.if ${MACHINE_ARCH} == "x86_64"
XEN_BUILD?=	amd64
.else
XEN_BUILD?=	${MACHINE_ARCH}
.endif
.endif

.if ${XEN_BUILD} == "i386"
XEN_MACHINE_ARCHS=	i386 x86 xen
.elif ${XEN_BUILD} == "amd64"
XEN_MACHINE_ARCHS=	amd64 x86 xen
.else
XEN_MACHINE_ARCHS=	${XEN_BUILD}
.endif
