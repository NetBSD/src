#	$NetBSD: Makefile.arch.inc,v 1.2.54.1 2007/10/17 21:08:14 bouyer Exp $

.if defined(MACHINE) && ${MACHINE} != "xen"
XEN_BUILD?=	${MACHINE}
.else
XEN_BUILD?=	${MACHINE_ARCH:Ui386}
.endif

.if ${XEN_BUILD} == "i386"
XEN_MACHINE_ARCHS=	i386 x86 xen
.else
XEN_MACHINE_ARCHS=	${XEN_BUILD}
.endif
