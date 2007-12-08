#	$NetBSD: Makefile.arch.inc,v 1.2.56.1 2007/12/08 18:18:16 mjf Exp $

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
