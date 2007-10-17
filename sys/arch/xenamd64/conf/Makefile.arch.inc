#	$NetBSD: Makefile.arch.inc,v 1.1.2.1 2007/10/17 21:38:20 bouyer Exp $

.if defined(MACHINE) && ${MACHINE} != "xenamd64"
XEN_BUILD?=	${MACHINE}
.else
XEN_BUILD?=	amd64
.endif

.if ${XEN_BUILD} == "amd64"
XEN_MACHINE_ARCHS=	amd64 x86 xen
.else
XEN_MACHINE_ARCHS=	${XEN_BUILD}
.endif
