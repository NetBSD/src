#	$NetBSD: Makefile.arch.inc,v 1.1.4.3 2004/09/18 14:42:43 skrll Exp $

.if defined(MACHINE) && ${MACHINE} != "xen"
XEN_BUILD?=	${MACHINE}
.else
XEN_BUILD?=	${MACHINE_ARCH:Ui386}
.endif

.if ${XEN_BUILD} == "i386"
XEN_MACHINE_ARCHS=	i386 x86
.else
XEN_MACHINE_ARCHS=	${XEN_BUILD}
.endif
