#	$NetBSD: Makefile.arch.inc,v 1.1.2.2 2004/05/22 15:59:21 he Exp $

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
